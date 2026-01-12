// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>

#include <cstdint>
#include <type_traits>
#include <vector>

#include <cute/int_tuple.hpp>
#include <cute/layout.hpp>
#include <cute/swizzle.hpp>
#include <cute/tensor.hpp>

#include <mdspan_cute/layout_cute.h>

using namespace mdspan_cute;

namespace {

// Build a few canonical cute layouts for unit tests
template <int M, int N> static auto make_static_2d_layout() {
  auto shape = cute::make_shape(cute::Int<M>{}, cute::Int<N>{});
  return cute::make_layout(shape);
}

static auto make_dynamic_1d_layout(std::size_t n) {
  auto shape = cute::make_shape(static_cast<int>(n));
  return cute::make_layout(shape);
}

static auto make_dynamic_2d_layout(std::size_t m, std::size_t n) {
  auto shape = cute::make_shape(static_cast<int>(m), static_cast<int>(n));
  return cute::make_layout(shape);
}

static auto make_dynamic_3d_layout(std::size_t m, std::size_t n,
                                   std::size_t k) {
  auto shape = cute::make_shape(static_cast<int>(m), static_cast<int>(n),
                                static_cast<int>(k));
  return cute::make_layout(shape);
}

// Iterate full coordinate space if small enough; otherwise sample
template <class Extents, class F>
void for_each_coord_safely(Extents const &exts, F &&f) {
  using size_t_ = typename Extents::index_type;
  const size_t_ rank = Extents::rank();
  std::vector<size_t_> sizes(rank);
  for (size_t_ i = 0; i < rank; ++i)
    sizes[i] = exts.extent(i);

  auto product = static_cast<std::size_t>(1);
  for (auto s : sizes)
    product *= static_cast<std::size_t>(s);

  auto call = [&](auto const &idx) { std::apply(f, idx); };

  if (product <= 256) {
    if constexpr (rank == 1) {
      for (size_t_ i = 0; i < sizes[0]; ++i)
        call(std::tuple<size_t_>(i));
    } else if constexpr (rank == 2) {
      for (size_t_ i = 0; i < sizes[0]; ++i)
        for (size_t_ j = 0; j < sizes[1]; ++j)
          call(std::tuple<size_t_, size_t_>(i, j));
    } else if constexpr (rank == 3) {
      for (size_t_ i = 0; i < sizes[0]; ++i)
        for (size_t_ j = 0; j < sizes[1]; ++j)
          for (size_t_ k = 0; k < sizes[2]; ++k)
            call(std::tuple<size_t_, size_t_, size_t_>(i, j, k));
    }
  } else {

    // Sample 128 random coordinates via RC
    for (int t = 0; t < 128; ++t) {
      if constexpr (rank == 1) {
        auto i = *rc::gen::inRange<std::size_t>(0, sizes[0]);
        call(std::tuple<size_t_>(i));
      } else if constexpr (rank == 2) {
        auto i = *rc::gen::inRange<std::size_t>(0, sizes[0]);
        auto j = *rc::gen::inRange<std::size_t>(0, sizes[1]);
        call(std::tuple<size_t_, size_t_>(i, j));
      } else if constexpr (rank == 3) {
        auto i = *rc::gen::inRange<std::size_t>(0, sizes[0]);
        auto j = *rc::gen::inRange<std::size_t>(0, sizes[1]);
        auto k = *rc::gen::inRange<std::size_t>(0, sizes[2]);
        call(std::tuple<size_t_, size_t_, size_t_>(i, j, k));
      }
    }
  }
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Type-trait sanity for static shapes
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("cute_to_extents preserves static arity", "[traits]") {
  using L1 = decltype(make_static_2d_layout<2, 3>());
  using S1 = cute_shape_t<L1>;
  using E1 = detail::cute_to_extents_t<std::size_t, S1>;
  static_assert(E1::rank() == 2);
  static_assert(E1::static_extent(0) == 2);
  static_assert(E1::static_extent(1) == 3);

  using L2 = decltype(cute::make_layout(cute::make_shape(cute::Int<4>{})));
  using S2 = cute_shape_t<L2>;
  using E2 = detail::cute_to_extents_t<std::size_t, S2>;
  static_assert(E2::rank() == 1);
  static_assert(E2::static_extent(0) == 4);
}

TEST_CASE("cute_to_extents preserves mixed static/dynamic", "[traits]") {
  auto cl = cute::make_layout(cute::make_shape(cute::Int<4>{}, 7));
  using CL = decltype(cl);
  using S = cute_shape_t<CL>;
  using E = detail::cute_to_extents_t<std::size_t, S>;
  static_assert(E::rank() == 2);
  static_assert(E::static_extent(0) == 4);
  static_assert(E::static_extent(1) == std::dynamic_extent);

  E ex = detail::make_extents_from_shape<E>(cute::shape(cl));
  REQUIRE(ex.extent(0) == 4);
  REQUIRE(ex.extent(1) == 7);
}

// ──────────────────────────────────────────────────────────────────────────────
// Mapping parity with cute layout (deterministic cases)
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("mapping parity: 2D static layout", "[mapping]") {

  auto cl = make_static_2d_layout<2, 3>();
  using CL = decltype(cl);
  using shape_t = cute_shape_t<CL>;
  using exts_t = detail::cute_to_extents_t<std::size_t, shape_t>;

  exts_t exts = detail::make_extents_from_shape<exts_t>(cute::shape(cl));
  layout_cute<CL>::template mapping<exts_t> m(exts, cl);

  REQUIRE(m.required_span_size() ==
          static_cast<typename exts_t::index_type>(cute::cosize(cl)));

  for_each_coord_safely(exts, [&](std::size_t i, std::size_t j) {
    auto off_cute = cl(cute::make_tuple(i, j));
    auto off_map = m(i, j);
    REQUIRE(off_map == static_cast<decltype(off_map)>(off_cute));
  });
}

TEST_CASE("mapping parity: 1D dynamic", "[mapping]") {
  auto cl = make_dynamic_1d_layout(37);
  using CL = decltype(cl);
  using shape_t = cute_shape_t<CL>;
  using exts_t = detail::cute_to_extents_t<std::size_t, shape_t>;
  exts_t exts = detail::make_extents_from_shape<exts_t>(cute::shape(cl));
  layout_cute<CL>::template mapping<exts_t> m(cl); // uses layout->extents ctor

  REQUIRE(m.extents().extent(0) == exts.extent(0));
  REQUIRE(m.required_span_size() ==
          static_cast<typename exts_t::index_type>(cute::cosize(cl)));

  for_each_coord_safely(exts, [&](std::size_t i) {
    auto off_cute = cl(i);
    auto off_map = m(i);
    REQUIRE(off_map == static_cast<decltype(off_map)>(off_cute));
  });
}

// ──────────────────────────────────────────────────────────────────────────────
// Swizzled layouts (non-strided, possibly non-exhaustive); parity still holds
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("swizzled 2D mapping parity", "[swizzle]") {
  auto base = make_dynamic_2d_layout(8, 8);
  auto shape = cute::shape(base);
  auto swz = cute::composition(swizzle::sw32{}, base); // simple row swizzle

  using CL = decltype(swz);
  using shape_t = cute_shape_t<CL>;
  using exts_t = detail::cute_to_extents_t<std::size_t, shape_t>;
  exts_t exts = detail::make_extents_from_shape<exts_t>(shape);

  layout_cute<CL>::template mapping<exts_t> m(exts, swz);

  REQUIRE(m.is_unique());
  REQUIRE(m.is_strided() == false);
  REQUIRE((m.is_exhaustive() == (cute::size(swz) == cute::cosize(swz))));

  for_each_coord_safely(exts, [&](std::size_t i, std::size_t j) {
    auto off_cute = swz(cute::make_tuple(i, j));
    REQUIRE(m(i, j) == static_cast<typename exts_t::index_type>(off_cute));
  });
}

// ──────────────────────────────────────────────────────────────────────────────
// Factory functions: as_mdspan / make_mdspan
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("make_mdspan: dynamic 2D", "[factory]") {
  auto cl = make_dynamic_2d_layout(7, 5);
  using CL = decltype(cl);
  using shape_t = cute_shape_t<CL>;
  using exts_t = detail::cute_to_extents_t<std::size_t, shape_t>;
  exts_t exts = detail::make_extents_from_shape<exts_t>(cute::shape(cl));

  std::vector<int> buf(cute::cosize(cl));
  auto md = make_mdspan(buf.data(), cl);

  REQUIRE(md.extents().extent(0) == exts.extent(0));
  REQUIRE(md.extents().extent(1) == exts.extent(1));

  // Fill by cute offset and verify mdspan accesses same storage
  for_each_coord_safely(exts, [&](std::size_t i, std::size_t j) {
    auto off = cl(cute::make_tuple(i, j));
    buf[off] = static_cast<int>(1000 + int(i) * 100 + int(j));
  });

  for_each_coord_safely(exts, [&](std::size_t i, std::size_t j) {
    auto expected = static_cast<int>(1000 + int(i) * 100 + int(j));
    REQUIRE(md[i, j] == expected);
  });
}

TEST_CASE("as_mdspan: cute::Tensor roundtrip", "[factory]") {
  auto cl = make_dynamic_3d_layout(3, 4, 5);
  std::vector<int> storage(cute::cosize(cl), -1);

  auto t = cute::make_tensor(storage.data(), cl);
  auto md = as_mdspan(t);

  REQUIRE(md.extent(0) == 3);
  REQUIRE(md.extent(1) == 4);
  REQUIRE(md.extent(2) == 5);

  // Write via mdspan and verify raw storage updated at cute offsets
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 4; ++j)
      for (std::size_t k = 0; k < 5; ++k)
        md[i, j, k] = int(i * 100 + j * 10 + k);

  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 4; ++j)
      for (std::size_t k = 0; k < 5; ++k) {
        auto off = cl(cute::make_tuple(i, j, k));
        REQUIRE(storage[off] == int(i * 100 + j * 10 + k));
      }
}

// ──────────────────────────────────────────────────────────────────────────────
// Property tests (RapidCheck) for dynamic ranks 1..3 (all-dynamic shapes)
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("mapping parity holds for dynamic rank-1", "[property][mapping]") {
  rc::prop("mapping parity holds for dynamic rank-1",
    [](const std::size_t n0_) {
      const std::size_t n0 = std::max<std::size_t>(1, n0_ % 64);
      auto cl = make_dynamic_1d_layout(n0);
      using CL = decltype(cl);
      using S = cute_shape_t<CL>;
      using E = detail::cute_to_extents_t<std::size_t, S>;
      E exts = detail::make_extents_from_shape<E>(cute::shape(cl));
      layout_cute<CL>::template mapping<E> m(exts, cl);

      RC_ASSERT(m.required_span_size() ==
                static_cast<typename E::index_type>(cute::cosize(cl)));

      for_each_coord_safely(exts, [&](std::size_t i) {
        auto off_cute = cl(i);
        auto off_map = m(i);
        RC_ASSERT(off_map == static_cast<typename E::index_type>(off_cute));
      });
    });
}

TEST_CASE("mapping parity holds for dynamic rank-2", "[property][mapping]") {
  rc::prop("mapping parity holds for dynamic rank-2",
    [](std::size_t m_, std::size_t n_) {
      const std::size_t m = std::max<std::size_t>(1, m_ % 16);
      const std::size_t n = std::max<std::size_t>(1, n_ % 16);
      auto cl = make_dynamic_2d_layout(m, n);
      using CL = decltype(cl);
      using S = cute_shape_t<CL>;
      using E = detail::cute_to_extents_t<std::size_t, S>;
      E exts = detail::make_extents_from_shape<E>(cute::shape(cl));
      layout_cute<CL>::template mapping<E> mapp(exts, cl);

      RC_ASSERT(mapp.required_span_size() ==
                static_cast<typename E::index_type>(cute::cosize(cl)));
      for_each_coord_safely(exts, [&](std::size_t i, std::size_t j) {
        RC_ASSERT(mapp(i, j) ==
                  static_cast<typename E::index_type>(cl(cute::make_tuple(i, j))));
      });
    });
}

TEST_CASE("mapping parity holds for dynamic rank-3", "[property][mapping]") {
  rc::prop("mapping parity holds for dynamic rank-3",
    [](std::size_t a_, std::size_t b_, std::size_t c_) {
      const std::size_t a = std::max<std::size_t>(1, a_ % 8);
      const std::size_t b = std::max<std::size_t>(1, b_ % 8);
      const std::size_t c = std::max<std::size_t>(1, c_ % 8);
      auto cl = make_dynamic_3d_layout(a, b, c);
      using CL = decltype(cl);
      using S = cute_shape_t<CL>;
      using E = detail::cute_to_extents_t<std::size_t, S>;
      E exts = detail::make_extents_from_shape<E>(cute::shape(cl));
      layout_cute<CL>::template mapping<E> mapp(exts, cl);

      RC_ASSERT(mapp.required_span_size() ==
                static_cast<typename E::index_type>(cute::cosize(cl)));
      for_each_coord_safely(exts, [&](std::size_t i, std::size_t j, std::size_t k) {
        RC_ASSERT(mapp(i, j, k) == static_cast<typename E::index_type>(
                                       cl(cute::make_tuple(i, j, k))));
      });
    });
}

// ──────────────────────────────────────────────────────────────────────────────
// Aliases compile sanity (static only)
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("cute_mdspan alias compiles for empty static layout", "[aliases]") {
  auto cl = make_static_2d_layout<2, 3>();
  using CL = decltype(cl);
  static_assert(cute_static_layout<CL> == std::is_empty_v<CL>);
  if constexpr (std::is_empty_v<CL>) {
    using MD = cute_mdspan<float, CL>;
    (void)sizeof(MD);
  } else {
    SUCCEED("Static alias skipped: layout type not empty on this toolchain.");
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Compile-time checks for nested shape flattening and extent extraction
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// Verify cute_extent_is_static_v and cute_static_extent_value work correctly
static_assert(mdspan_cute::detail::cute_extent_is_static_v<cute::Int<5>>);
static_assert(mdspan_cute::detail::cute_extent_is_static_v<cute::Int<7>>);
static_assert(!mdspan_cute::detail::cute_extent_is_static_v<int>);
static_assert(
    mdspan_cute::detail::cute_static_extent_value<cute::Int<5>>::value == 5);
static_assert(
    mdspan_cute::detail::cute_static_extent_value<cute::Int<7>>::value == 7);
static_assert(
    mdspan_cute::detail::cute_static_extent_value<cute::Int<0>>::value == 0);

// Verify nested shape flattening works correctly
using NestedShape1 = cute::tuple<cute::Int<4>, cute::tuple<cute::Int<8>, int>>;
using FlatShape1 = mdspan_cute::detail::shape_flatten_t<NestedShape1>;
static_assert(
    std::same_as<FlatShape1, cute::tuple<cute::Int<4>, cute::Int<8>, int>>);

using NestedShape2 = cute::tuple<cute::Int<2>, cute::tuple<cute::Int<3>, int>>;
using FlatShape2 = mdspan_cute::detail::shape_flatten_t<NestedShape2>;
static_assert(
    std::same_as<FlatShape2, cute::tuple<cute::Int<2>, cute::Int<3>, int>>);

// Verify extents are correctly derived from flattened shape
using ExtentsFromFlat1 =
    mdspan_cute::detail::cute_to_extents_t<std::size_t, FlatShape1>;
static_assert(ExtentsFromFlat1::rank() == 3);
static_assert(ExtentsFromFlat1::static_extent(0) == 4);
static_assert(ExtentsFromFlat1::static_extent(1) == 8);
static_assert(ExtentsFromFlat1::static_extent(2) == std::dynamic_extent);

using ExtentsFromFlat2 =
    mdspan_cute::detail::cute_to_extents_t<std::size_t, FlatShape2>;
static_assert(ExtentsFromFlat2::rank() == 3);
static_assert(ExtentsFromFlat2::static_extent(0) == 2);
static_assert(ExtentsFromFlat2::static_extent(1) == 3);
static_assert(ExtentsFromFlat2::static_extent(2) == std::dynamic_extent);

// Verify simple shapes still work
using SimpleShape = cute::tuple<cute::Int<16>, cute::Int<8>>;
using SimpleExtents =
    mdspan_cute::detail::cute_to_extents_t<std::size_t, SimpleShape>;
static_assert(SimpleExtents::rank() == 2);
static_assert(SimpleExtents::static_extent(0) == 16);
static_assert(SimpleExtents::static_extent(1) == 8);
static_assert(SimpleExtents::rank_dynamic() == 0);

// Verify dynamic extents
using DynShape = cute::tuple<int, cute::Int<8>, int>;
using DynExtents =
    mdspan_cute::detail::cute_to_extents_t<std::size_t, DynShape>;
static_assert(DynExtents::rank() == 3);
static_assert(DynExtents::static_extent(0) == std::dynamic_extent);
static_assert(DynExtents::static_extent(1) == 8);
static_assert(DynExtents::static_extent(2) == std::dynamic_extent);
static_assert(DynExtents::rank_dynamic() == 2);

// Verify 1D shapes
using Shape1D = cute::Int<32>;
using Extents1D = mdspan_cute::detail::cute_to_extents_t<std::size_t, Shape1D>;
static_assert(Extents1D::rank() == 1);
static_assert(Extents1D::static_extent(0) == 32);

// Verify cute_layout_flat_rank_v
using TestLayout = cute::Layout<cute::tuple<cute::Int<4>, cute::Int<8>>>;
static_assert(mdspan_cute::detail::cute_layout_flat_rank_v<TestLayout> == 2);

} // anonymous namespace
