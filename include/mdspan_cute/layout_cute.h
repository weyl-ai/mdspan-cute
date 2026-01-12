// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright 2026 Weyl AI
//
// mdspan_cute/layout_cute.h
//
// Zero-overhead bridge between std::mdspan layout policies and CUTLASS cute
// layouts. Enables C++23 tensor[i,j,k] syntax with cute's polyhedral tensor
// algebra.

#pragma once

// GCC 15 / CUDA compatibility - must come before cute headers
#include <mdspan_cute/cuda_gcc15_compat.h>

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <experimental/mdspan>
#include <type_traits>
#include <utility>

// cute headers
#include <cute/int_tuple.hpp>
#include <cute/layout.hpp>
#include <cute/swizzle.hpp>
#include <cute/tensor.hpp>

namespace mdspan_cute {

// Import mdspan types from experimental namespace
namespace stdex = std::experimental;
using stdex::extents;
using stdex::mdspan;
using stdex::layout_left;
using stdex::layout_right;
using stdex::layout_stride;

// These are in std, not std::experimental
using std::dextents;
using std::dynamic_extent;

// ═══════════════════════════════════════════════════════════════════════════════
// Concepts and helpers for cute/mdspan interop
// ═══════════════════════════════════════════════════════════════════════════════

template <typename L>
using cute_shape_t = decltype(cute::shape(std::declval<L const &>()));

template <typename T>
concept cute_layout = requires(T layout, std::size_t i) {
  { cute::size(layout) } -> std::convertible_to<std::size_t>;
  { cute::cosize(layout) } -> std::convertible_to<std::size_t>;
  { cute::shape(layout) };
  requires(
      requires { layout(i); } || requires { layout(cute::make_tuple(i)); });
};

template <typename T>
concept cute_static_layout = cute_layout<T> && std::is_empty_v<T>;

// ═══════════════════════════════════════════════════════════════════════════════
namespace detail {

// ─────────────────────────────────────────────────────────────────────────────
// Robust compile-time extraction from cute::Int<N>
// Avoid ODR/ADL pitfalls by using a trait instead of constexpr conversion
// ─────────────────────────────────────────────────────────────────────────────

template <typename T> struct cute_static_extent_value; // undefined by default

template <int N>
struct cute_static_extent_value<cute::Int<N>>
    : std::integral_constant<std::size_t, static_cast<std::size_t>(N)> {};

template <typename T> inline constexpr bool cute_extent_is_static_v = false;

template <int N>
inline constexpr bool cute_extent_is_static_v<cute::Int<N>> = true;

// Helper to get extent value: static for cute::Int<N>, dynamic for integral types
// Use std::integral_constant to force immediate evaluation in template contexts
template <typename T, typename = void>
struct cute_extent_value_or_dynamic : std::integral_constant<std::size_t, std::dynamic_extent> {};

template <typename T>
struct cute_extent_value_or_dynamic<T, std::enable_if_t<cute_extent_is_static_v<std::remove_cvref_t<T>>>>
    : std::integral_constant<std::size_t, cute_static_extent_value<std::remove_cvref_t<T>>::value> {};

// ─────────────────────────────────────────────────────────────────────────────
// Nested-shape flattening (compile-time type-level)
// Flattens nested cute::tuple shapes for proper extent extraction
// ─────────────────────────────────────────────────────────────────────────────

template <class T> struct shape_flatten {
  using type =
      cute::tuple<T>; // integral or cute::Int<N> → single-element tuple
};

template <typename... Ts> struct shape_flatten<cute::tuple<Ts...>> {
  using type = decltype(cute::tuple_cat(typename shape_flatten<Ts>::type{}...));
};

template <class Shape>
using shape_flatten_t = typename shape_flatten<std::remove_cvref_t<Shape>>::type;

// ─────────────────────────────────────────────────────────────────────────────
// Nested-shape flattening (runtime value-level)
// ─────────────────────────────────────────────────────────────────────────────

template <class T>
constexpr auto flatten_shape(T const &v)
  requires std::is_integral_v<T>
{
  return cute::make_tuple(v);
}

template <int N> constexpr auto flatten_shape(cute::Int<N> const &) {
  return cute::make_tuple(cute::Int<N>{});
}

template <class Tuple, std::size_t... Is>
constexpr auto flatten_tuple_impl(Tuple const &t, std::index_sequence<Is...>) {
  return cute::tuple_cat(flatten_shape(cute::get<Is>(t))...);
}

template <class... Ts>
constexpr auto flatten_shape(cute::tuple<Ts...> const &t) {
  return flatten_tuple_impl(t, std::index_sequence_for<Ts...>{});
}

// ─────────────────────────────────────────────────────────────────────────────
// Extract extent at dimension I from cute shape
// ─────────────────────────────────────────────────────────────────────────────

template <std::size_t I, typename Shape> struct cute_extent_at_impl;

template <std::size_t I, typename T>
  requires(I == 0 && std::is_integral_v<T>)
struct cute_extent_at_impl<I, T> {
  static constexpr auto get(T shape) { return shape; }
};

template <std::size_t I, int N>
  requires(I == 0)
struct cute_extent_at_impl<I, cute::Int<N>> {
  static constexpr auto get(cute::Int<N>) { return N; }
};

template <std::size_t I, typename... Ts>
  requires(I < sizeof...(Ts))
struct cute_extent_at_impl<I, cute::tuple<Ts...>> {
  static constexpr auto get(cute::tuple<Ts...> const &shape) {
    return cute::get<I>(shape);
  }
};

// Wrapper that strips cv-qualifiers automatically
template <std::size_t I, typename Shape>
struct cute_extent_at : cute_extent_at_impl<I, std::remove_cvref_t<Shape>> {};

// ─────────────────────────────────────────────────────────────────────────────
// Convert cute Int<N> to std::size_t; pass through integral values
// ─────────────────────────────────────────────────────────────────────────────

constexpr std::size_t to_size_t(std::size_t v) { return v; }

template <typename T>
  requires std::is_integral_v<T> && (!std::is_same_v<T, std::size_t>)
constexpr std::size_t to_size_t(T v) {
  return static_cast<std::size_t>(v);
}

template <int N> constexpr std::size_t to_size_t(cute::Int<N>) {
  return static_cast<std::size_t>(N);
}

// ─────────────────────────────────────────────────────────────────────────────
// Build mdspan extents type from cute shape
// Use cute_extent_is_static_v and cute_static_extent_value for safety
// ─────────────────────────────────────────────────────────────────────────────

template <typename IndexType, typename CuteShape, typename = void>
struct cute_to_extents;

// Single integral dimension (dynamic)
template <typename IndexType, typename T>
  requires std::is_integral_v<T>
struct cute_to_extents<IndexType, T> {
  using type = std::extents<IndexType, std::dynamic_extent>;
};

// Single static dimension
template <typename IndexType, int N>
struct cute_to_extents<IndexType, cute::Int<N>> {
  using type =
      std::extents<IndexType, cute_static_extent_value<cute::Int<N>>::value>;
};

// Helper to build extent list
template <typename IndexType, std::size_t... Vals>
struct make_extents_helper {
  using type = std::extents<IndexType, Vals...>;
};

// Tuple of dimensions - preserve static where possible using the safe trait
template <typename IndexType, typename... Ts>
struct cute_to_extents<IndexType, cute::tuple<Ts...>> {
  using type = typename make_extents_helper<IndexType, cute_extent_value_or_dynamic<Ts>::value...>::type;
};

template <typename IndexType, typename CuteShape>
using cute_to_extents_t = typename cute_to_extents<IndexType, std::remove_cvref_t<CuteShape>>::type;

// ─────────────────────────────────────────────────────────────────────────────
// Build a concrete extents value from a cute shape (fills only dynamic dims)
// ─────────────────────────────────────────────────────────────────────────────

template <std::size_t I, class Extents, class Shape>
constexpr void fill_dyn_at(
    std::array<typename Extents::index_type, Extents::rank_dynamic()> &dst,
    std::size_t &di, Shape const &shape) {
  if constexpr (Extents::static_extent(I) == std::dynamic_extent) {
    const auto v = to_size_t(cute_extent_at<I, Shape>::get(shape));
    dst[di++] = static_cast<typename Extents::index_type>(v);
  }
}

template <class Extents, class Shape, std::size_t... Is>
constexpr Extents make_extents_from_shape_impl(Shape const &shape,
                                               std::index_sequence<Is...>) {
  static_assert(sizeof...(Is) == Extents::rank(),
                "Rank mismatch between Extents and shape");
  if constexpr (Extents::rank_dynamic() == 0) {
    return Extents{};
  } else {
    std::array<typename Extents::index_type, Extents::rank_dynamic()> dyn{};
    std::size_t di = 0;
    (fill_dyn_at<Is, Extents>(dyn, di, shape), ...);
    auto make = [&]<std::size_t... Js>(std::index_sequence<Js...>) {
      return Extents(dyn[Js]...);
    };
    return make(std::make_index_sequence<Extents::rank_dynamic()>{});
  }
}

template <class Extents, class Shape>
constexpr Extents make_extents_from_shape(Shape const &shape) {
  return make_extents_from_shape_impl<Extents>(
      shape, std::make_index_sequence<Extents::rank()>{});
}

// ─────────────────────────────────────────────────────────────────────────────
// Concept for cute layouts that expose strides
// ─────────────────────────────────────────────────────────────────────────────

template <typename L>
concept has_cute_stride = requires(L const &l, std::size_t i) {
  { cute::stride(l, i) } -> std::convertible_to<std::size_t>;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concepts for 1D operator() dispatch (scalar vs tuple callable)
// ─────────────────────────────────────────────────────────────────────────────

template <class L, class I>
concept cute_callable_scalar = requires(L const &l, I i) { l(i); };

template <class L, class I>
concept cute_callable_tuple =
    requires(L const &l, I i) { l(cute::make_tuple(i)); };

// ─────────────────────────────────────────────────────────────────────────────
// Compute flattened rank of a CuteLayout at compile time
// ─────────────────────────────────────────────────────────────────────────────

template <typename CuteLayout>
inline constexpr std::size_t cute_layout_flat_rank_v =
    cute::tuple_size<std::remove_cvref_t<shape_flatten_t<cute_shape_t<CuteLayout>>>>::value;

} // namespace detail

// ═══════════════════════════════════════════════════════════════════════════════
// layout_cute: The mdspan layout policy wrapping a cute::Layout
// ═══════════════════════════════════════════════════════════════════════════════

template <cute_layout CuteLayout> struct layout_cute {

  template <typename Extents> class mapping {
  public:
    using extents_type = Extents;
    using index_type = typename extents_type::index_type;
    using size_type = typename extents_type::size_type;
    using rank_type = typename extents_type::rank_type;
    using layout_type = layout_cute;

  private:
    [[no_unique_address]] extents_type extents_{};
    [[no_unique_address]] CuteLayout cute_layout_{};

  public:
    // ─────────────────────────────────────────────────────────────────────
    // Constructors
    // Gated on default_initializable for layouts that aren't
    // default-constructible
    // ─────────────────────────────────────────────────────────────────────

    constexpr mapping() noexcept
      requires std::default_initializable<CuteLayout>
    = default;

    constexpr mapping(mapping const &) noexcept = default;
    constexpr mapping(mapping &&) noexcept = default;
    constexpr mapping &operator=(mapping const &) noexcept = default;
    constexpr mapping &operator=(mapping &&) noexcept = default;

    // Construct from extents (uses default cute layout)
    constexpr explicit mapping(extents_type const &ext) noexcept
      requires std::default_initializable<CuteLayout>
        : extents_(ext), cute_layout_{} {}

    // Construct from extents and cute layout
    // Includes compile-time rank check
    constexpr mapping(extents_type const &ext,
                      CuteLayout const &layout) noexcept
        : extents_(ext), cute_layout_(layout) {
      // Compile-time rank check
      constexpr std::size_t layout_rank =
          detail::cute_layout_flat_rank_v<CuteLayout>;
      static_assert(
          extents_type::rank() == layout_rank,
          "mdspan_cute::layout_cute: rank(extents) != rank(shape(layout))");

      // Debug-only runtime check for dynamic extent mismatches
#if !defined(NDEBUG)
      auto const shape_flat = detail::flatten_shape(cute::shape(cute_layout_));
      auto check_dyn = [this, &shape_flat]<std::size_t... Is>(
                           std::index_sequence<Is...>) {
        ((extents_type::static_extent(Is) == std::dynamic_extent
              ? (void)assert(
                    extents_.extent(Is) ==
                    detail::to_size_t(
                        detail::cute_extent_at<Is, decltype(shape_flat)>::get(
                            shape_flat)))
              : (void)0),
         ...);
      };
      check_dyn(std::make_index_sequence<extents_type::rank()>{});
#endif
    }

    // Construct from cute layout alone (derive extents from its shape)
    constexpr explicit mapping(CuteLayout const &layout) noexcept
        : extents_(detail::make_extents_from_shape<extents_type>(
              detail::flatten_shape(cute::shape(layout)))),
          cute_layout_(layout) {}

    // ─────────────────────────────────────────────────────────────────────
    // Observers
    // ─────────────────────────────────────────────────────────────────────
    [[nodiscard]] constexpr auto extents() const noexcept
        -> extents_type const & {
      return extents_;
    }

    [[nodiscard]] constexpr auto cute_layout() const noexcept
        -> CuteLayout const & {
      return cute_layout_;
    }

    // required_span_size: use cute's cosize for non-contiguous layouts
    // Returns size_type per mdspan mapping requirements
    [[nodiscard]] constexpr auto required_span_size() const noexcept
        -> size_type {
      return static_cast<size_type>(cute::cosize(cute_layout_));
    }

    // ─────────────────────────────────────────────────────────────────────
    // Mapping operator - THE CORE BRIDGE
    // Three overloads: 1D scalar, 1D tuple fallback, ND tuple
    // ─────────────────────────────────────────────────────────────────────

    // 1D – scalar call preferred when available
    template <typename I0>
      requires(extents_type::rank() == 1) &&
              std::is_convertible_v<I0, index_type> &&
              detail::cute_callable_scalar<CuteLayout, index_type>
    [[nodiscard]] constexpr index_type operator()(I0 i0) const noexcept {
      return static_cast<index_type>(cute_layout_(static_cast<index_type>(i0)));
    }

    // 1D – tuple fallback when scalar is not available
    template <typename I0>
      requires(extents_type::rank() == 1) &&
              std::is_convertible_v<I0, index_type> &&
              (!detail::cute_callable_scalar<CuteLayout, index_type>) &&
              detail::cute_callable_tuple<CuteLayout, index_type>
    [[nodiscard]] constexpr index_type operator()(I0 i0) const noexcept {
      return static_cast<index_type>(
          cute_layout_(cute::make_tuple(static_cast<index_type>(i0))));
    }

    // ND (rank >= 2) – always use tuple form
    template <typename... Indices>
      requires(sizeof...(Indices) == extents_type::rank()) &&
              (extents_type::rank() >= 2) &&
              (std::is_convertible_v<Indices, index_type> && ...)
    [[nodiscard]] constexpr index_type
    operator()(Indices... indices) const noexcept {
      return static_cast<index_type>(
          cute_layout_(cute::make_tuple(static_cast<index_type>(indices)...)));
    }

    // ─────────────────────────────────────────────────────────────────────
    // Layout mapping properties
    // ─────────────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr bool is_always_unique() noexcept {
      return true;
    }
    [[nodiscard]] static constexpr bool is_always_exhaustive() noexcept {
      return false;
    }
    [[nodiscard]] static constexpr bool is_always_strided() noexcept {
      return false;
    }

    // Contiguous traits (conservative: always false)
    // Many cute compositions (e.g., swizzles) are not mdspan-contiguous
    [[nodiscard]] static constexpr bool is_always_contiguous() noexcept {
      return false;
    }

    [[nodiscard]] constexpr bool is_unique() const noexcept { return true; }
    [[nodiscard]] constexpr bool is_exhaustive() const noexcept {
      return cute::size(cute_layout_) == cute::cosize(cute_layout_);
    }

    // is_contiguous() - conservative default
    [[nodiscard]] constexpr bool is_contiguous() const noexcept {
      return false;
    }

    // is_strided() - true when strides are available
    [[nodiscard]] constexpr bool is_strided() const noexcept {
      if constexpr (detail::has_cute_stride<CuteLayout>)
        return true;
      else
        return false;
    }

    // Optional stride(i) when the cute layout exposes strides
    [[nodiscard]] constexpr index_type stride(rank_type r) const
        noexcept(noexcept(cute::stride(std::declval<CuteLayout const &>(),
                                       std::size_t{})))
      requires detail::has_cute_stride<CuteLayout>
    {
      return static_cast<index_type>(
          cute::stride(cute_layout_, static_cast<std::size_t>(r)));
    }

    // ─────────────────────────────────────────────────────────────────────
    // Equality semantics
    // Only enabled when CuteLayout is equality-comparable (proper semantics)
    // ─────────────────────────────────────────────────────────────────────

    template <typename OtherExtents>
    [[nodiscard]] friend constexpr auto
    operator==(mapping const &lhs, mapping<OtherExtents> const &rhs) noexcept
        -> bool
      requires std::equality_comparable<CuteLayout>
    {
      return lhs.extents_ == rhs.extents_ &&
             lhs.cute_layout_ == rhs.cute_layout_;
    }

    // Note: No fallback for non-equality-comparable layouts.
    // Comparing only extents would violate mapping equality semantics.

    // ─────────────────────────────────────────────────────────────────────
    // Swap (for completeness)
    // ─────────────────────────────────────────────────────────────────────
    friend void swap(mapping &a, mapping &b) noexcept {
      using std::swap;
      swap(a.extents_, b.extents_);
      swap(a.cute_layout_, b.cute_layout_);
    }
  };
};

// ═══════════════════════════════════════════════════════════════════════════════
// as_mdspan: Convert cute::Tensor to std::mdspan
// Preserves const/volatile from tensor.data() pointer type
// Uses flattened shape for proper nested tuple handling
// ═══════════════════════════════════════════════════════════════════════════════

template <typename Engine, typename Layout>
[[nodiscard]] constexpr auto
as_mdspan(cute::Tensor<Engine, Layout> const &tensor) {
  using element_ptr = decltype(tensor.data());
  using element_type = std::remove_pointer_t<element_ptr>;
  using cute_layout_type = Layout;
  using raw_shape_t = cute_shape_t<cute_layout_type>;
  using flat_shape_t = detail::shape_flatten_t<raw_shape_t>;
  using extents_type = detail::cute_to_extents_t<std::size_t, flat_shape_t>;
  using layout_policy = layout_cute<cute_layout_type>;

  auto const &cl = tensor.layout();
  auto const shape_raw = cute::shape(cl);
  auto const shape_flat = detail::flatten_shape(shape_raw);
  extents_type exts = detail::make_extents_from_shape<extents_type>(shape_flat);

  return std::mdspan<element_type, extents_type, layout_policy>(
      tensor.data(),
      typename layout_policy::template mapping<extents_type>(exts, cl));
}

// Non-const overload to preserve mutability
template <typename Engine, typename Layout>
[[nodiscard]] constexpr auto as_mdspan(cute::Tensor<Engine, Layout> &tensor) {
  using element_ptr = std::remove_cvref_t<decltype(tensor.data())>;
  using element_type = std::remove_pointer_t<element_ptr>;
  using cute_layout_type = Layout;
  using raw_shape_t = cute_shape_t<cute_layout_type>;
  using flat_shape_t = detail::shape_flatten_t<raw_shape_t>;
  using extents_type = detail::cute_to_extents_t<std::size_t, flat_shape_t>;
  using layout_policy = layout_cute<cute_layout_type>;

  auto const &cl = tensor.layout();
  auto const shape_raw = cute::shape(cl);
  auto const shape_flat = detail::flatten_shape(shape_raw);
  extents_type exts = detail::make_extents_from_shape<extents_type>(shape_flat);

  return std::mdspan<element_type, extents_type, layout_policy>(
      tensor.data(),
      typename layout_policy::template mapping<extents_type>(exts, cl));
}

template <typename T, cute_layout CuteLayout>
[[nodiscard]] constexpr auto make_mdspan(T *ptr, CuteLayout const &layout) {
  using raw_shape_t = cute_shape_t<CuteLayout>;
  using flat_shape_t = detail::shape_flatten_t<raw_shape_t>;
  using extents_type = detail::cute_to_extents_t<std::size_t, flat_shape_t>;
  using layout_policy = layout_cute<CuteLayout>;

  auto const shape_raw = cute::shape(layout);
  auto const shape_flat = detail::flatten_shape(shape_raw);
  extents_type exts = detail::make_extents_from_shape<extents_type>(shape_flat);

  return std::mdspan<T, extents_type, layout_policy>(
      ptr,
      typename layout_policy::template mapping<extents_type>(exts, layout));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Swizzle Presets
// ═══════════════════════════════════════════════════════════════════════════════

namespace swizzle {

using sw128 = cute::Swizzle<3, 3, 3>;
using sw64 = cute::Swizzle<2, 3, 3>;
using sw32 = cute::Swizzle<1, 3, 3>;

template <typename SwizzleType, typename Shape, typename Stride>
[[nodiscard]] constexpr auto make_swizzled_layout(Shape const &shape,
                                                  Stride const &stride) {
  return cute::composition(SwizzleType{}, cute::make_layout(shape, stride));
}

template <typename SwizzleType, typename Shape>
[[nodiscard]] constexpr auto make_swizzled_layout(Shape const &shape) {
  return cute::composition(SwizzleType{}, cute::make_layout(shape));
}

} // namespace swizzle

// ═══════════════════════════════════════════════════════════════════════════════
// Convenience Type Aliases
// ═══════════════════════════════════════════════════════════════════════════════

template <typename T, cute_static_layout CuteLayout>
using cute_mdspan = std::mdspan<
    T,
    detail::cute_to_extents_t<
        std::size_t, detail::shape_flatten_t<cute_shape_t<CuteLayout>>>,
    layout_cute<CuteLayout>>;

template <typename T, std::size_t Rank, cute_layout CuteLayout>
using cute_dmdspan =
    std::mdspan<T, std::dextents<std::size_t, Rank>, layout_cute<CuteLayout>>;

} // namespace mdspan_cute
