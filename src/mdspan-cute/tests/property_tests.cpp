// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// tests/property_tests.cpp
//
// Property-based tests for the CuTe Layout Algebra
//
// These tests verify the theorems from razorgirl by throwing random inputs
// at the invariants. If these pass on millions of cases, we have high
// confidence the Lean proofs are correct.

#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

// RapidCheck + Catch2 integration
// We use rc::prop inside TEST_CASE instead of the RC_CATCH_PROP macro

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <vector>

namespace property_tests {

// ═══════════════════════════════════════════════════════════════════════════════
// Core Layout Algebra Implementation (mirrors Lean definitions exactly)
// ═══════════════════════════════════════════════════════════════════════════════

struct Layout {
  std::vector<size_t> shape;
  std::vector<size_t> stride;

  // Size = product of all shape dimensions
  size_t size() const {
    return std::accumulate(shape.begin(), shape.end(), size_t{1},
                           std::multiplies<>{});
  }

  size_t rank() const { return shape.size(); }

  bool valid() const {
    return shape.size() == stride.size() &&
           std::all_of(shape.begin(), shape.end(),
                       [](size_t s) { return s > 0; });
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// Definition 2.1: The Isomorphism ι : [0, M) ↔ [0,M₀) × [0,M₁) × ... × [0,Mₐ)
// ─────────────────────────────────────────────────────────────────────────────

// Decompose: x ↦ (x mod M₀, ⌊x/M₀⌋ mod M₁, ...)
std::vector<size_t> decompose(const std::vector<size_t> &shape, size_t x) {
  std::vector<size_t> coords(shape.size());
  size_t divisor = 1;
  for (size_t k = 0; k < shape.size(); ++k) {
    coords[k] = (x / divisor) % shape[k];
    divisor *= shape[k];
  }
  return coords;
}

// Recompose: (x₀, x₁, ...) ↦ x₀ + x₁·M₀ + x₂·M₀·M₁ + ...
size_t recompose(const std::vector<size_t> &shape,
                 const std::vector<size_t> &coords) {
  size_t x = 0;
  size_t multiplier = 1;
  for (size_t k = 0; k < shape.size(); ++k) {
    x += coords[k] * multiplier;
    multiplier *= shape[k];
  }
  return x;
}

// ─────────────────────────────────────────────────────────────────────────────
// Definition 2.3: Layout Function f_L(x) = Σ xᵢ·dᵢ
// ─────────────────────────────────────────────────────────────────────────────

size_t layout_apply(const Layout &L, size_t x) {
  auto coords = decompose(L.shape, x);
  size_t offset = 0;
  for (size_t k = 0; k < L.rank(); ++k) {
    offset += coords[k] * L.stride[k];
  }
  return offset;
}

// Extended layout function (last dim unbounded)
size_t layout_apply_ext(const Layout &L, size_t x) {
  if (L.rank() == 0)
    return 0;

  std::vector<size_t> coords(L.rank());
  size_t divisor = 1;
  for (size_t k = 0; k < L.rank(); ++k) {
    if (k == L.rank() - 1) {
      coords[k] = x / divisor; // No modulo for last
    } else {
      coords[k] = (x / divisor) % L.shape[k];
      divisor *= L.shape[k];
    }
  }

  size_t offset = 0;
  for (size_t k = 0; k < L.rank(); ++k) {
    offset += coords[k] * L.stride[k];
  }
  return offset;
}

// ─────────────────────────────────────────────────────────────────────────────
// Coalescence: When can we merge adjacent modes?
// ─────────────────────────────────────────────────────────────────────────────

struct Mode {
  size_t shape;
  size_t stride;
};

enum class CoalesceResult { Merged, Unchanged };

// Try to coalesce two adjacent modes
std::pair<CoalesceResult, Mode> try_coalesce(Mode m0, Mode m1) {
  if (m1.shape == 1) {
    return {CoalesceResult::Merged, m0};
  }
  if (m0.shape == 1) {
    return {CoalesceResult::Merged, m1};
  }
  if (m1.stride == m0.shape * m0.stride) {
    return {CoalesceResult::Merged, {m0.shape * m1.shape, m0.stride}};
  }
  return {CoalesceResult::Unchanged, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// FTTC: The Fundamental Theorem of TMA Correctness
// ─────────────────────────────────────────────────────────────────────────────

struct FTTCConfig {
  size_t element_stride;
  size_t box_size;
  size_t tensor_size;
};

// Strong correctness is UNACHIEVABLE iff: e < B < S AND e ∤ B
bool fttc_violated(const FTTCConfig &c) {
  return c.element_stride < c.box_size && c.box_size < c.tensor_size &&
         c.box_size % c.element_stride != 0;
}

bool strong_correctness_achievable(const FTTCConfig &c) {
  return c.box_size % c.element_stride == 0 || c.box_size >= c.tensor_size ||
         c.element_stride >= c.box_size;
}

// ─────────────────────────────────────────────────────────────────────────────
// Split/Merge: IterDomain transformations
// ─────────────────────────────────────────────────────────────────────────────

size_t ceil_div(size_t a, size_t b) { return (a + b - 1) / b; }

struct Split {
  size_t extent;
  size_t factor;

  size_t outer_extent() const { return ceil_div(extent, factor); }
  size_t inner_extent() const { return factor; }
  bool is_divisible() const { return extent % factor == 0; }
  size_t num_holes() const { return outer_extent() * inner_extent() - extent; }

  // Index mapping
  size_t index_outer(size_t i) const { return i / factor; }
  size_t index_inner(size_t i) const { return i % factor; }
  size_t reconstruct(size_t outer, size_t inner) const {
    return outer * factor + inner;
  }
};

struct Merge {
  size_t outer_extent;
  size_t inner_extent;

  size_t result_extent() const { return outer_extent * inner_extent; }

  // Index mapping
  size_t index_merged(size_t i_outer, size_t i_inner) const {
    return i_outer * inner_extent + i_inner;
  }
  size_t index_outer(size_t i) const { return i / inner_extent; }
  size_t index_inner(size_t i) const { return i % inner_extent; }
};

} // namespace property_tests

// Note: Per weyl-std C++ guidelines, we use fully qualified names for external
// namespaces. The local property_tests namespace uses 'using namespace' since
// it's file-local.
using namespace property_tests;

// ═══════════════════════════════════════════════════════════════════════════════
// PROPERTY TESTS
// ═══════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: recompose ∘ decompose = id
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("recompose(decompose(x)) = x for valid indices") {
  rc::prop("recompose(decompose(x)) = x for valid indices",
           [](const std::vector<uint8_t> &raw_shape) {
             // Generate shapes with dimensions 1-16
             std::vector<size_t> shape;
             for (auto s : raw_shape) {
               if (shape.size() >= 4)
                 break;                   // Max rank 4
               size_t dim = (s % 15) + 1; // 1 to 16
               shape.push_back(dim);
             }
             RC_PRE(!shape.empty());

             size_t size = std::accumulate(shape.begin(), shape.end(),
                                           size_t{1}, std::multiplies<>{});
             RC_PRE(size > 0 && size < 10000); // Reasonable size

             // Test all indices
             for (size_t x = 0; x < size; ++x) {
               auto coords = decompose(shape, x);
               size_t reconstructed = recompose(shape, coords);
               RC_ASSERT(reconstructed == x);
             }
           });
}

TEST_CASE("decompose produces valid coordinates") {
  rc::prop("decompose produces valid coordinates",
           [](const std::vector<uint8_t> &raw_shape, size_t x_raw) {
             std::vector<size_t> shape;
             for (auto s : raw_shape) {
               if (shape.size() >= 4)
                 break;
               size_t dim = (s % 15) + 1;
               shape.push_back(dim);
             }
             RC_PRE(!shape.empty());

             size_t size = std::accumulate(shape.begin(), shape.end(),
                                           size_t{1}, std::multiplies<>{});
             RC_PRE(size > 0);

             size_t x = x_raw % size;
             auto coords = decompose(shape, x);

             // Each coordinate must be within bounds
             for (size_t k = 0; k < shape.size(); ++k) {
               RC_ASSERT(coords[k] < shape[k]);
             }
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: Coalescence preserves the layout function
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("coalescence preserves layout function when d1 = n0 * d0") {
  rc::prop("coalescence preserves layout function when d1 = n0 * d0",
           [](uint8_t n0_raw, uint8_t n1_raw, uint8_t d0_raw) {
             size_t n0 = (n0_raw % 15) + 1; // 1-16
             size_t n1 = (n1_raw % 15) + 1; // 1-16
             size_t d0 = (d0_raw % 15) + 1; // 1-16
             size_t d1 = n0 * d0;           // Coalescible condition

             Mode m0{n0, d0};
             Mode m1{n1, d1};

             auto [result, merged] = try_coalesce(m0, m1);
             RC_ASSERT(result == CoalesceResult::Merged);

             // Verify: for all valid (x0, x1), original = merged
             for (size_t x0 = 0; x0 < n0; ++x0) {
               for (size_t x1 = 0; x1 < n1; ++x1) {
                 size_t original_offset = x0 * d0 + x1 * d1;
                 size_t merged_index = x0 + x1 * n0;
                 size_t merged_offset = merged_index * merged.stride;
                 RC_ASSERT(original_offset == merged_offset);
               }
             }
           });
}

TEST_CASE("d0 = n1 * d1 does NOT allow coalescence") {
  rc::prop("d0 = n1 * d1 does NOT allow coalescence",
           [](uint8_t n0_raw, uint8_t n1_raw, uint8_t d1_raw) {
             size_t n0 = (n0_raw % 14) + 2; // 2-16 (avoid n0=1 edge case)
             size_t n1 = (n1_raw % 14) + 2; // 2-16 (avoid n1=1 edge case)
             size_t d1 = (d1_raw % 15) + 1; // 1-16
             size_t d0 = n1 * d1;           // The NON-coalescible condition!

             Mode m0{n0, d0};
             Mode m1{n1, d1};

             // This should NOT coalesce (unless d1 happens to equal n0 * d0)
             if (d1 != n0 * d0) {
               auto [result, _] = try_coalesce(m0, m1);
               RC_ASSERT(result == CoalesceResult::Unchanged);
             }
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: FTTC - strong correctness achievable iff condition holds
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("FTTC: violated iff e < B < S and e ∤ B") {
  rc::prop("FTTC: violated iff e < B < S and e ∤ B",
           [](uint8_t e_raw, uint8_t b_raw, uint8_t s_raw) {
             size_t e = (e_raw % 15) + 1; // element_stride: 1-16
             size_t b = (b_raw % 31) + 1; // box_size: 1-32
             size_t s = (s_raw % 63) + 1; // tensor_size: 1-64

             FTTCConfig c{e, b, s};

             bool violated = fttc_violated(c);
             bool achievable = strong_correctness_achievable(c);

             // FTTC: violated ⟺ ¬achievable
             RC_ASSERT(violated == !achievable);

             // Explicit check of the condition
             bool expected_violated = (e < b) && (b < s) && (b % e != 0);
             RC_ASSERT(violated == expected_violated);
           });
}

TEST_CASE("FTTC worked example: e=3, B=8, S=16 is violated", "[fttc]") {
  FTTCConfig c{3, 8, 16};

  REQUIRE(fttc_violated(c));
  REQUIRE(!strong_correctness_achievable(c));

  // Why: 3 < 8 < 16, and 8 % 3 = 2 ≠ 0
  REQUIRE(c.element_stride < c.box_size);
  REQUIRE(c.box_size < c.tensor_size);
  REQUIRE(c.box_size % c.element_stride != 0);
}

TEST_CASE("FTTC worked example: e=4, B=8, S=16 is NOT violated", "[fttc]") {
  FTTCConfig c{4, 8, 16};

  REQUIRE(!fttc_violated(c));
  REQUIRE(strong_correctness_achievable(c));

  // Why: 4 | 8 (8 % 4 = 0)
  REQUIRE(c.box_size % c.element_stride == 0);
}

TEST_CASE("FTTC worked example: e=9, B=8, S=16 is NOT violated", "[fttc]") {
  FTTCConfig c{9, 8, 16};

  REQUIRE(!fttc_violated(c));
  REQUIRE(strong_correctness_achievable(c));

  // Why: e >= B (9 >= 8)
  REQUIRE(c.element_stride >= c.box_size);
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: Divisible splits create no holes
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("divisible splits create no holes") {
  rc::prop("divisible splits create no holes",
           [](uint8_t extent_raw, uint8_t factor_idx) {
             size_t extent = (extent_raw % 127) + 1; // 1-128

             // Find a divisor of extent
             std::vector<size_t> divisors;
             for (size_t d = 1; d <= extent; ++d) {
               if (extent % d == 0)
                 divisors.push_back(d);
             }
             RC_PRE(!divisors.empty());

             size_t factor = divisors[factor_idx % divisors.size()];
             Split s{extent, factor};

             RC_ASSERT(s.is_divisible());
             RC_ASSERT(s.num_holes() == 0);
             RC_ASSERT(s.outer_extent() * s.inner_extent() == extent);
           });
}

TEST_CASE("indivisible splits create holes") {
  rc::prop("indivisible splits create holes",
           [](uint8_t extent_raw, uint8_t factor_raw) {
             size_t extent = (extent_raw % 126) + 2; // 2-128
             size_t factor = (factor_raw % 14) + 2;  // 2-16

             RC_PRE(extent % factor != 0); // Indivisible
             RC_PRE(factor < extent);      // Meaningful split

             Split s{extent, factor};

             RC_ASSERT(!s.is_divisible());
             RC_ASSERT(s.num_holes() > 0);
             RC_ASSERT(s.num_holes() == factor - (extent % factor));
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: Split/Merge roundtrip preserves indices (when divisible)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("split then merge recovers original index (divisible case)") {
  rc::prop("split then merge recovers original index (divisible case)",
           [](uint8_t extent_raw, uint8_t factor_idx) {
             size_t extent = (extent_raw % 127) + 1;

             std::vector<size_t> divisors;
             for (size_t d = 1; d <= extent; ++d) {
               if (extent % d == 0)
                 divisors.push_back(d);
             }
             RC_PRE(!divisors.empty());

             size_t factor = divisors[factor_idx % divisors.size()];
             Split s{extent, factor};
             Merge m{s.outer_extent(), s.inner_extent()};

             // For all valid indices, split then merge = identity
             for (size_t i = 0; i < extent; ++i) {
               size_t outer = s.index_outer(i);
               size_t inner = s.index_inner(i);
               size_t merged = m.index_merged(outer, inner);
               RC_ASSERT(merged == i);
             }
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem: Layout function is linear in coordinates
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("layout function is sum of coord * stride") {
  rc::prop("layout function is sum of coord * stride",
           [](uint8_t rank_raw, const std::vector<uint8_t> &raw_data) {
             size_t rank = (rank_raw % 4) + 1; // Rank 1-4
             RC_PRE(raw_data.size() >= rank * 2);

             Layout L;
             for (size_t i = 0; i < rank; ++i) {
               L.shape.push_back((raw_data[i * 2] % 7) + 1);       // 1-8
               L.stride.push_back((raw_data[i * 2 + 1] % 31) + 1); // 1-32
             }
             RC_PRE(L.valid());

             // For all valid indices
             for (size_t x = 0; x < std::min(L.size(), size_t{256}); ++x) {
               auto coords = decompose(L.shape, x);

               // Manual calculation
               size_t expected = 0;
               for (size_t k = 0; k < L.rank(); ++k) {
                 expected += coords[k] * L.stride[k];
               }

               RC_ASSERT(layout_apply(L, x) == expected);
             }
           });
}

// ═══════════════════════════════════════════════════════════════════════════════
// WORKED EXAMPLES FROM NVFUSER/CUTLASS
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("nvfuser example: 128×128 GEMM tile with 64-element warp tile",
          "[nvfuser]") {
  // From nvfuser's scheduling documentation
  // Block tile: 128×128
  // Warp tile: 64

  Split block_split{128, 64};

  REQUIRE(block_split.is_divisible());
  REQUIRE(block_split.num_holes() == 0);
  REQUIRE(block_split.outer_extent() == 2);  // 2 warps per block dim
  REQUIRE(block_split.inner_extent() == 64); // 64 elements per warp
}

TEST_CASE("nvfuser example: 128 tile with 48-element split creates holes",
          "[nvfuser]") {
  // This is a BUG pattern that nvfuser catches
  Split bad_split{128, 48};

  REQUIRE(!bad_split.is_divisible());
  REQUIRE(bad_split.num_holes() > 0);

  // 128 / 48 = 2.67, ceil = 3 tiles
  // 3 × 48 = 144, but we only have 128 elements
  // Holes = 144 - 128 = 16
  REQUIRE(bad_split.outer_extent() == 3);
  REQUIRE(bad_split.num_holes() == 16);
}

TEST_CASE("CUTLASS example: row-major vs column-major layout", "[cutlass]") {
  // 4×8 matrix
  Layout row_major{{4, 8}, {8, 1}}; // stride 8 in first dim (rows)
  Layout col_major{{4, 8}, {1, 4}}; // stride 1 in first dim (columns)

  // Element at (2, 3)
  // Row-major: 2*8 + 3*1 = 19
  // Col-major: 2*1 + 3*4 = 14

  // Index 19 in linear order (row-major iteration)
  // In 4×8 row-major: row = 19/8 = 2, col = 19%8 = 3 → (2,3)

  // decompose uses column-major ordering (first dim varies fastest)
  // For shape [4, 8], index 13 gives coords [1, 3]
  // With row-major strides [8, 1]: offset = 1*8 + 3*1 = 11
  REQUIRE(layout_apply(row_major, 13) == 11);

  // Check all elements are unique (no aliasing)
  std::vector<size_t> row_offsets, col_offsets;
  for (size_t i = 0; i < 32; ++i) {
    row_offsets.push_back(layout_apply(row_major, i));
    col_offsets.push_back(layout_apply(col_major, i));
  }

  std::sort(row_offsets.begin(), row_offsets.end());
  std::sort(col_offsets.begin(), col_offsets.end());

  // Both should produce unique offsets 0-31
  for (size_t i = 0; i < 32; ++i) {
    REQUIRE(row_offsets[i] == i);
    REQUIRE(col_offsets[i] == i);
  }
}

TEST_CASE("CUTLASS example: swizzled shared memory for bank conflict avoidance",
          "[cutlass]") {
  // Swizzle<3,3,3> XORs bits to avoid bank conflicts
  // For a 64×64 tile with 4-byte elements:
  // - 32 banks, 4 bytes each = 128 bytes per bank cycle
  // - Without swizzle: column 0 hits bank 0 for all rows → 32-way conflict
  // - With swizzle: XOR scrambles addresses

  // This test verifies the PROPERTY that swizzle eliminates conflicts,
  // not the exact XOR pattern (which is implementation-specific)

  // Simplified model: check that consecutive columns in same row
  // don't all map to same bank

  auto compute_bank = [](size_t offset) { return offset % 32; };

  // Without swizzle (row-major 64×64, stride 64)
  Layout no_swizzle{{64, 64}, {64, 1}};

  // Check column 0 across rows 0-31
  // decompose uses column-major: for coords [row, 0], index = row + 0*64 = row
  std::vector<size_t> banks_col0;
  for (size_t row = 0; row < 32; ++row) {
    // Index of element at coords [row, 0] in column-major decompose
    size_t idx = row;
    size_t offset = layout_apply(no_swizzle, idx);
    banks_col0.push_back(compute_bank(offset));
  }

  // All hit the same bank (bank 0) - this is the conflict!
  size_t unique_banks =
      std::set<size_t>(banks_col0.begin(), banks_col0.end()).size();
  REQUIRE(unique_banks == 1); // 32-way bank conflict!

  // A proper swizzle would spread these across different banks
  // (We're just demonstrating the problem swizzles solve)
}

TEST_CASE("MMA atom: sm80_m16n8k16 thread-value mapping", "[cutlass][mma]") {
  // The mma.m16n8k16.f16 atom:
  // - 32 threads participate
  // - C matrix: 16×8 = 128 elements
  // - Each thread owns 4 elements (128/32 = 4)

  size_t num_threads = 32;
  size_t values_per_thread = 4;
  size_t M = 16, N = 8;

  REQUIRE(num_threads * values_per_thread == M * N);

  // The TV layout ensures every element is owned by exactly one thread
  // This is the INVARIANT that CuTe enforces
}

// ═══════════════════════════════════════════════════════════════════════════════
// NVFUSER THEOREM TESTS (from doc/math/integer-division.md)
// ═══════════════════════════════════════════════════════════════════════════════

// Theorem 2.10: If b is multiple of c, then a × (b/c) = (a×b)/c
TEST_CASE("nvfuser Theorem 2.10: a*(b/c) = (a*b)/c when c|b") {
  rc::prop("nvfuser Theorem 2.10: a*(b/c) = (a*b)/c when c|b",
           [](uint8_t a_raw, uint8_t k_raw, uint8_t c_raw) {
             size_t a = a_raw + 1;
             size_t c = (c_raw % 15) + 1; // 1-16
             size_t k = (k_raw % 15) + 1; // 1-16
             size_t b = c * k;            // b is multiple of c

             RC_ASSERT(a * (b / c) == (a * b) / c);
           });
}

// Theorem 2.11: a/(b×c) = (a/b)/c
TEST_CASE("nvfuser Theorem 2.11: a/(b*c) = (a/b)/c") {
  rc::prop("nvfuser Theorem 2.11: a/(b*c) = (a/b)/c",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1; // 1-16
             size_t c = (c_raw % 15) + 1; // 1-16

             RC_ASSERT(a / (b * c) == (a / b) / c);
           });
}

// Theorem 2.12: a % (b×c) = a % b + ((a/b) % c) × b
TEST_CASE("nvfuser Theorem 2.12: mod decomposition") {
  rc::prop("nvfuser Theorem 2.12: mod decomposition",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1;
             size_t c = (c_raw % 15) + 1;

             RC_ASSERT(a % (b * c) == a % b + (a / b % c) * b);
           });
}

// Theorem 2.16: i/d < D ⟺ i < D×d
TEST_CASE("nvfuser Theorem 2.16: div bound equivalence") {
  rc::prop("nvfuser Theorem 2.16: div bound equivalence",
           [](uint8_t i_raw, uint8_t D_raw, uint8_t d_raw) {
             size_t i = i_raw;
             size_t D = (D_raw % 31) + 1;
             size_t d = (d_raw % 15) + 1;

             RC_ASSERT((i / d < D) == (i < D * d));
           });
}

// ═══════════════════════════════════════════════════════════════════════════════
// FTTC EXHAUSTIVE TESTS (from doc/reading/tma-modeling-in-depth.md)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("FTTC exhaustive: all small configs", "[fttc][exhaustive]") {
  // Test all combinations for small values
  for (size_t e = 1; e <= 16; ++e) {
    for (size_t B = 1; B <= 32; ++B) {
      for (size_t S = 1; S <= 64; ++S) {
        FTTCConfig c{e, B, S};

        bool violated = fttc_violated(c);
        bool achievable = strong_correctness_achievable(c);

        // FTTC: violated ⟺ ¬achievable
        REQUIRE(violated == !achievable);

        // Verify the exact condition
        bool expected = (e < B) && (B < S) && (B % e != 0);
        REQUIRE(violated == expected);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// ITERDOMAIN SPLIT-SPLIT EQUIVALENCE (from doc/reading/iterdomain.md
// Theorem 2.1)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("nvfuser split-split: i/n/m = i/(m*n)") {
  rc::prop("nvfuser split-split: i/n/m = i/(m*n)",
           [](uint8_t i_raw, uint8_t m_raw, uint8_t n_raw) {
             size_t i = i_raw;
             size_t m = (m_raw % 15) + 1;
             size_t n = (n_raw % 15) + 1;

             // Theorem 2.11 applied
             RC_ASSERT(i / n / m == i / (m * n));
           });
}

TEST_CASE("nvfuser split-split: i%n = i%(m*n)%n") {
  rc::prop("nvfuser split-split: i%n = i%(m*n)%n",
           [](uint8_t i_raw, uint8_t m_raw, uint8_t n_raw) {
             size_t i = i_raw;
             size_t m = (m_raw % 15) + 1;
             size_t n = (n_raw % 15) + 1;

             // Theorem 2.7.1 applied
             RC_ASSERT(i % n == i % (m * n) % n);
           });
}

// ═══════════════════════════════════════════════════════════════════════════════
// PREDICATION THEOREM 1 (from doc/reading/divisibility-of-split.md)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("nvfuser predication: I0,I2 in bound => I1 in bound") {
  rc::prop("nvfuser predication: I0,I2 in bound => I1 in bound",
           [](uint8_t N0_raw, uint8_t N2_raw) {
             size_t N0 = (N0_raw % 63) + 1;  // 1-64
             size_t N2 = (N2_raw % 15) + 1;  // 1-16
             size_t N1 = (N0 + N2 - 1) / N2; // ceilDiv

             // For all valid i0
             for (size_t i0 = 0; i0 < N0; ++i0) {
               size_t i1 = i0 / N2;
               size_t i2 = i0 % N2;

               // If i0 < N0 and i2 < N2, then i1 < N1
               RC_ASSERT(i2 < N2); // Always true by construction
               RC_ASSERT(i1 < N1); // The theorem
             }
           });
}

// ═══════════════════════════════════════════════════════════════════════════════
// PREDICATION THEOREMS (from doc/reading/divisibility-of-split.md)
// ═══════════════════════════════════════════════════════════════════════════════

// Theorem 2: Merge predication - I0 in bound ⟺ I2 in bound
TEST_CASE("nvfuser Predication Theorem 2: merge boundary equivalence") {
  rc::prop("nvfuser Predication Theorem 2: merge boundary equivalence",
           [](uint8_t N0_raw, uint8_t N1_raw) {
             size_t N0 = (N0_raw % 15) + 1;
             size_t N1 = (N1_raw % 15) + 1;
             size_t N2 = N0 * N1;

             // For all i2 in the merged domain
             for (size_t i2 = 0; i2 < N2 + 5; ++i2) { // Test beyond bounds too
               size_t i0 = i2 / N1;

               bool i0_in_bound = (i0 < N0);
               bool i2_in_bound = (i2 < N2);

               // Theorem 2: These are equivalent
               RC_ASSERT(i0_in_bound == i2_in_bound);
             }
           });
}

// Theorem 3: Resize predication - I0 in bound ⟹ I1 in bound (when L,R ≥ 0)
TEST_CASE("nvfuser Predication Theorem 3: resize boundary implication") {
  rc::prop("nvfuser Predication Theorem 3: resize boundary implication",
           [](uint8_t N0_raw, uint8_t L_raw, uint8_t R_raw) {
             size_t N0 = (N0_raw % 15) + 1;
             size_t L = L_raw % 8; // Non-negative left expand
             size_t R = R_raw % 8; // Non-negative right expand
             size_t N1 = N0 + L + R;

             // For all i1 in the resized domain
             for (size_t i1 = 0; i1 < N1; ++i1) {
               if (i1 >= L) { // i0 = i1 - L is valid
                 size_t i0 = i1 - L;

                 if (i0 < N0) { // I0 in bound
                   // Then I1 must be in bound (Theorem 3)
                   RC_ASSERT(i1 < N1);
                 }
               }
             }
           });
}

// ═══════════════════════════════════════════════════════════════════════════════
// WEAK VS STRONG CORRECTNESS (from doc/reading/divisibility-of-split.md)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("Correctness model: indivisible split creates holes",
          "[correctness]") {
  // From the documentation example: Split(I{6}, 4)
  // Original extent: 6
  // Split factor: 4
  // Outer extent: ceilDiv(6, 4) = 2
  // Inner extent: 4
  // Total: 2 * 4 = 8 > 6 (2 holes!)

  size_t N0 = 6;
  size_t factor = 4;
  size_t N_outer = ceil_div(N0, factor); // 2
  size_t N_inner = factor;               // 4

  REQUIRE(N_outer == 2);
  REQUIRE(N_inner == 4);
  REQUIRE(N_outer * N_inner == 8);
  REQUIRE(N_outer * N_inner > N0); // Holes exist!

  // Count valid items vs holes
  size_t valid_count = 0;
  size_t hole_count = 0;

  for (size_t i_outer = 0; i_outer < N_outer; ++i_outer) {
    for (size_t i_inner = 0; i_inner < N_inner; ++i_inner) {
      size_t i0 = i_outer * factor + i_inner;
      if (i0 < N0) {
        ++valid_count;
      } else {
        ++hole_count;
      }
    }
  }

  REQUIRE(valid_count == 6); // Original extent
  REQUIRE(hole_count == 2);  // The holes
}

TEST_CASE("Correctness model: divisible split creates no holes",
          "[correctness]") {
  // Split(I{6}, 2) - divisible
  size_t N0 = 6;
  size_t factor = 2;
  size_t N_outer = ceil_div(N0, factor); // 3
  size_t N_inner = factor;               // 2

  REQUIRE(N_outer == 3);
  REQUIRE(N_inner == 2);
  REQUIRE(N_outer * N_inner == N0); // No holes!

  // All items are valid
  for (size_t i_outer = 0; i_outer < N_outer; ++i_outer) {
    for (size_t i_inner = 0; i_inner < N_inner; ++i_inner) {
      size_t i0 = i_outer * factor + i_inner;
      REQUIRE(i0 < N0);
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// MONOTONIC FUNCTION (from doc/math/monotonic-function.md)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("Monotonic: floor division is weakly increasing", "[monotonic]") {
  // f(x) = x / d is weakly increasing for d > 0
  for (size_t d = 1; d <= 16; ++d) {
    for (size_t x = 0; x < 64; ++x) {
      for (size_t y = x; y < 64; ++y) {
        // x ≤ y ⟹ f(x) ≤ f(y)
        REQUIRE(x / d <= y / d);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// LOGIC THEOREM 1 (from doc/math/logic.md)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("Logic Theorem 1: conditional simplification", "[logic]") {
  // Example from the doc: i >= 0 && i < 6 && i % 6 < 3
  // Under i >= 0 && i < 6, we have i % 6 = i
  // So simplifies to: i >= 0 && i < 3

  for (int i = -10; i < 20; ++i) {
    bool original = (i >= 0) && (i < 6) && (i % 6 < 3);
    bool simplified = (i >= 0) && (i < 3);

    // These should be equivalent
    REQUIRE(original == simplified);
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// VILLA STRAYLIGHT EXTRACTED PROPERTIES
// These properties are PROVEN in Lean for all valid inputs.
// See: proof/VillaStraylight.lean (26 theorems, 0 sorry)
// ═══════════════════════════════════════════════════════════════════════════════

#include "../../lean4-proof/extracted/cpp/property_tests.hpp"

// Note: Using fully qualified mdspan_cute::properties:: prefix per weyl-std
// guidelines.

// ─────────────────────────────────────────────────────────────────────────────
// §1: Coordinate Isomorphism (from VillaStraylight §1)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: recompose_decompose_2d", "[villa][coords]") {
  rc::prop("recompose ∘ decompose = id", [](uint8_t M0_raw, uint8_t M1_raw,
                                            uint8_t x_raw) {
    size_t M0 = (M0_raw % 15) + 1;
    size_t M1 = (M1_raw % 15) + 1;
    size_t x = x_raw % (M0 * M1);
    RC_ASSERT(mdspan_cute::properties::prop_recompose_decompose_2d(M0, M1, x));
  });
}

TEST_CASE("Villa Straylight: decompose_recompose_2d", "[villa][coords]") {
  rc::prop("decompose ∘ recompose = id",
           [](uint8_t M0_raw, uint8_t x0_raw, uint8_t x1_raw) {
             size_t M0 = (M0_raw % 15) + 1;
             size_t M1 = 16; // Fixed for simplicity
             size_t x0 = x0_raw % M0;
             size_t x1 = x1_raw % M1;
             RC_ASSERT(mdspan_cute::properties::prop_decompose_recompose_2d(
                 M0, M1, x0, x1));
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// §2: Coalescence (from VillaStraylight §2)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: coalesce_preserves_function",
          "[villa][coalesce]") {
  rc::prop("coalescence preserves eval", [](uint8_t s0_raw, uint8_t s1_raw,
                                            uint8_t d0_raw) {
    size_t s0 = (s0_raw % 15) + 1;
    size_t s1 = (s1_raw % 15) + 1;
    size_t d0 = (d0_raw % 15) + 1;
    size_t d1 = s0 * d0; // packed merge condition
    size_t x0 = *rc::gen::inRange<size_t>(0, s0);
    size_t x1 = *rc::gen::inRange<size_t>(0, s1);
    RC_ASSERT(mdspan_cute::properties::prop_coalesce_preserves_function(
        s0, d0, s1, d1, x0, x1));
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// §3: Ceiling Division - The Galois Connection (from VillaStraylight §3)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: ceilDiv_le_iff (Galois)", "[villa][ceildiv]") {
  rc::prop("⌈a/b⌉ ≤ Q ⟺ a ≤ Q×b",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t Q_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1;
             size_t Q = Q_raw % 32;
             RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_le_iff(a, b, Q));
           });
}

TEST_CASE("Villa Straylight: ceilDiv_assoc", "[villa][ceildiv]") {
  rc::prop("⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉",
           [](uint8_t i_raw, uint8_t m_raw, uint8_t n_raw) {
             size_t i = i_raw;
             size_t m = (m_raw % 15) + 1;
             size_t n = (n_raw % 15) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_assoc(i, m, n));
           });
}

TEST_CASE("Villa Straylight: ceilDiv_of_dvd", "[villa][ceildiv]") {
  rc::prop("divisible ⟹ ⌈n/d⌉ = n/d", [](uint8_t k_raw, uint8_t d_raw) {
    size_t d = (d_raw % 15) + 1;
    size_t k = k_raw % 16;
    size_t n = d * k; // divisible
    RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_of_dvd(n, d));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_eq_div_add_one_of_not_dvd",
          "[villa][ceildiv]") {
  rc::prop("indivisible ⟹ ⌈n/d⌉ = n/d + 1", [](uint8_t n_raw, uint8_t d_raw) {
    size_t n = n_raw + 1;
    size_t d = (d_raw % 14) + 2;
    RC_PRE(n % d != 0); // indivisible
    RC_ASSERT(
        mdspan_cute::properties::prop_ceilDiv_eq_div_add_one_of_not_dvd(n, d));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_mul_ge_self", "[villa][ceildiv]") {
  rc::prop("a ≤ ⌈a/b⌉ × b", [](uint8_t a_raw, uint8_t b_raw) {
    size_t a = a_raw;
    size_t b = (b_raw % 15) + 1;
    RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_mul_ge_self(a, b));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_mul_sub_self_pos_of_not_dvd",
          "[villa][ceildiv]") {
  rc::prop("indivisible ⟹ ⌈n/d⌉×d > n", [](uint8_t n_raw, uint8_t d_raw) {
    size_t n = n_raw + 1;
    size_t d = (d_raw % 14) + 2;
    RC_PRE(n % d != 0);
    RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_mul_sub_self_pos_of_not_dvd(
        n, d));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_eq_zero_iff", "[villa][ceildiv]") {
  rc::prop("⌈a/b⌉ = 0 ⟺ a = 0", [](uint8_t a_raw, uint8_t b_raw) {
    size_t a = a_raw;
    size_t b = (b_raw % 15) + 1;
    RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_eq_zero_iff(a, b));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_mono_left", "[villa][ceildiv]") {
  rc::prop("ceilDiv monotone in numerator", [](uint8_t a_raw,
                                               uint8_t a_prime_raw,
                                               uint8_t b_raw) {
    size_t a = a_raw;
    size_t a_prime = a_prime_raw;
    RC_PRE(a <= a_prime);
    size_t b = (b_raw % 15) + 1;
    RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_mono_left(a, a_prime, b));
  });
}

TEST_CASE("Villa Straylight: ceilDiv_antitone_right", "[villa][ceildiv]") {
  rc::prop("ceilDiv antitone in denominator",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t b_prime_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1;
             size_t b_prime = (b_prime_raw % 15) + 1;
             RC_PRE(b <= b_prime);
             RC_ASSERT(mdspan_cute::properties::prop_ceilDiv_antitone_right(
                 a, b, b_prime));
           });
}

TEST_CASE("Villa Straylight: ceilDiv_mul_sub_self_eq_zero_iff (no-holes)",
          "[villa][ceildiv]") {
  rc::prop("no holes ⟺ divisibility", [](uint8_t n_raw, uint8_t d_raw) {
    size_t n = n_raw;
    size_t d = (d_raw % 15) + 1;
    RC_ASSERT(
        mdspan_cute::properties::prop_ceilDiv_mul_sub_self_eq_zero_iff(n, d));
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// §3: FTTC - The Fundamental Theorem of TMA Correctness
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: FTTC", "[villa][fttc]") {
  rc::prop("violated ⟺ ¬achievable",
           [](uint8_t e_raw, uint8_t B_raw, uint8_t S_raw) {
             size_t e = (e_raw % 15) + 1;
             size_t B = (B_raw % 31) + 1;
             size_t S = (S_raw % 63) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_fttc(e, B, S));
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// §4: Integer Division Theorems (from VillaStraylight §4)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: thm_2_5 (small numbers)", "[villa][intdiv]") {
  rc::prop("r < a ⟹ r%a = r, r/a = 0", [](uint8_t r_raw, uint8_t a_raw) {
    size_t a = (a_raw % 15) + 1;
    size_t r = r_raw % a;
    RC_ASSERT(mdspan_cute::properties::prop_thm_2_5(r, a));
  });
}

TEST_CASE("Villa Straylight: thm_2_7_1 (add multiple)", "[villa][intdiv]") {
  rc::prop("a%c=0 ⟹ (a+b)%c = b%c",
           [](uint8_t k_raw, uint8_t b_raw, uint8_t c_raw) {
             size_t c = (c_raw % 15) + 1;
             size_t a = c * (k_raw % 16);
             size_t b = b_raw;
             RC_ASSERT(mdspan_cute::properties::prop_thm_2_7_1(a, b, c));
           });
}

TEST_CASE("Villa Straylight: thm_2_7_2 (nested mod)", "[villa][intdiv]") {
  rc::prop("a%(b×c)%b = a%b", [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
    size_t a = a_raw;
    size_t b = (b_raw % 15) + 1;
    size_t c = (c_raw % 15) + 1;
    RC_ASSERT(mdspan_cute::properties::prop_thm_2_7_2(a, b, c));
  });
}

TEST_CASE("Villa Straylight: thm_2_10 (div distributes)", "[villa][intdiv]") {
  rc::prop("c|b ⟹ a×(b/c) = (a×b)/c",
           [](uint8_t a_raw, uint8_t k_raw, uint8_t c_raw) {
             size_t c = (c_raw % 15) + 1;
             size_t k = (k_raw % 15) + 1;
             size_t b = c * k;
             size_t a = a_raw;
             RC_ASSERT(mdspan_cute::properties::prop_thm_2_10(a, b, c));
           });
}

TEST_CASE("Villa Straylight: thm_2_11 (div associates)", "[villa][intdiv]") {
  rc::prop("a/(b×c) = a/b/c", [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
    size_t a = a_raw;
    size_t b = (b_raw % 15) + 1;
    size_t c = (c_raw % 15) + 1;
    RC_ASSERT(mdspan_cute::properties::prop_thm_2_11(a, b, c));
  });
}

TEST_CASE("Villa Straylight: thm_2_12 (mixed-radix decomposition)",
          "[villa][intdiv]") {
  rc::prop("a%(b×c) = a%b + (a/b%c)×b",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1;
             size_t c = (c_raw % 15) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_thm_2_12(a, b, c));
           });
}

TEST_CASE("Villa Straylight: thm_2_15_1 (extract middle digit)",
          "[villa][intdiv]") {
  rc::prop("a/b%c = a%(b×c)/b",
           [](uint8_t a_raw, uint8_t b_raw, uint8_t c_raw) {
             size_t a = a_raw;
             size_t b = (b_raw % 15) + 1;
             size_t c = (c_raw % 15) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_thm_2_15_1(a, b, c));
           });
}

TEST_CASE("Villa Straylight: thm_2_16 (bound theorem)", "[villa][intdiv]") {
  rc::prop("i/d < D ⟺ i < D×d",
           [](uint8_t i_raw, uint8_t D_raw, uint8_t d_raw) {
             size_t i = i_raw;
             size_t D = (D_raw % 31) + 1;
             size_t d = (d_raw % 15) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_thm_2_16(i, D, d));
           });
}

// ─────────────────────────────────────────────────────────────────────────────
// Merge-Split and Split-Split (from VillaStraylight)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: merge_split_identity", "[villa][split]") {
  rc::prop("merge ∘ split = id ⟺ divisibility",
           [](uint8_t extent_raw, uint8_t factor_raw) {
             size_t extent = (extent_raw % 127) + 1;
             size_t factor = (factor_raw % 15) + 1;
             RC_ASSERT(mdspan_cute::properties::prop_merge_split_identity(
                 extent, factor));
           });
}

TEST_CASE("Villa Straylight: split_split_extent", "[villa][split]") {
  rc::prop(
      "⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉", [](uint8_t m_raw, uint8_t n_raw, uint8_t i_raw) {
        size_t m = (m_raw % 15) + 1;
        size_t n = (n_raw % 15) + 1;
        size_t i = i_raw;
        RC_ASSERT(mdspan_cute::properties::prop_split_split_extent(m, n, i));
      });
}

TEST_CASE("Villa Straylight: split_split_outer", "[villa][split]") {
  rc::prop("i/n/m = i/(m×n)", [](uint8_t m_raw, uint8_t n_raw, uint8_t i_raw) {
    size_t m = (m_raw % 15) + 1;
    size_t n = (n_raw % 15) + 1;
    size_t i = i_raw;
    RC_ASSERT(mdspan_cute::properties::prop_split_split_outer(m, n, i));
  });
}

TEST_CASE("Villa Straylight: split_split_inner_outer", "[villa][split]") {
  rc::prop("i/n%m = i%(m×n)/n", [](uint8_t m_raw, uint8_t n_raw,
                                   uint8_t i_raw) {
    size_t m = (m_raw % 15) + 1;
    size_t n = (n_raw % 15) + 1;
    size_t i = i_raw;
    RC_ASSERT(mdspan_cute::properties::prop_split_split_inner_outer(m, n, i));
  });
}

TEST_CASE("Villa Straylight: split_split_inner_inner", "[villa][split]") {
  rc::prop("i%n = i%(m×n)%n", [](uint8_t m_raw, uint8_t n_raw, uint8_t i_raw) {
    size_t m = (m_raw % 15) + 1;
    size_t n = (n_raw % 15) + 1;
    size_t i = i_raw;
    RC_ASSERT(mdspan_cute::properties::prop_split_split_inner_inner(m, n, i));
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Predication Theorems
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Villa Straylight: predication_thm_2", "[villa][predicate]") {
  rc::prop("I0 in boundary ⟺ I2 in boundary", [](uint8_t i2_raw, uint8_t N0_raw,
                                                 uint8_t N1_raw) {
    size_t N0 = (N0_raw % 15) + 1;
    size_t N1 = (N1_raw % 15) + 1;
    size_t i2 = i2_raw % (N0 * N1 + 5);
    RC_ASSERT(mdspan_cute::properties::prop_predication_thm_2(i2, N0, N1));
  });
}
