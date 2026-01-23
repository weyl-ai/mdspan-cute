/*
 * property_tests_standalone.cpp - Standalone property tests (no dependencies)
 *
 * AUTO-GENERATED FROM VillaStraylight.lean
 * DO NOT EDIT - regenerate with `lake exe extract-properties`
 *
 * Compile: g++ -std=c++20 -O2 property_tests_standalone.cpp -o property_tests
 * Run:     ./property_tests
 *
 * Uses a simple PRNG and exhaustive small-value testing.
 * No external dependencies required.
 */

#include "property_tests.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>

using namespace mdspan_cute::properties;

// ============================================================================
// Minimal test harness
// ============================================================================

static size_t tests_run = 0;
static size_t tests_passed = 0;
static size_t tests_failed = 0;

#define TEST(name)                                                             \
  static void test_##name();                                                   \
  static struct Test_##name {                                                  \
    Test_##name() {                                                            \
      printf("  %-50s ", #name);                                               \
      fflush(stdout);                                                          \
      size_t before = tests_failed;                                            \
      test_##name();                                                           \
      if (tests_failed == before) {                                            \
        printf("\033[32mPASS\033[0m\n");                                       \
      }                                                                        \
    }                                                                          \
  } test_instance_##name;                                                      \
  static void test_##name()

#define CHECK(expr)                                                            \
  do {                                                                         \
    tests_run++;                                                               \
    if (expr) {                                                                \
      tests_passed++;                                                          \
    } else {                                                                   \
      tests_failed++;                                                          \
      printf("\033[31mFAIL\033[0m\n    %s:%d: %s\n", __FILE__, __LINE__,       \
             #expr);                                                           \
    }                                                                          \
  } while (0)

#define SECTION(name) printf("\n[%s]\n", name);

// ============================================================================
// Random testing
// ============================================================================

static std::mt19937_64 rng;

void init_rng() {
  auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
  rng.seed(static_cast<uint64_t>(seed));
}

size_t rand_pos(size_t max = 1000) {
  std::uniform_int_distribution<size_t> dist(1, max);
  return dist(rng);
}

size_t rand_nat(size_t max = 10000) {
  std::uniform_int_distribution<size_t> dist(0, max);
  return dist(rng);
}

size_t rand_bounded(size_t bound) {
  if (bound == 0)
    return 0;
  std::uniform_int_distribution<size_t> dist(0, bound - 1);
  return dist(rng);
}

// ============================================================================
// Property tests
// ============================================================================

constexpr size_t ITERATIONS = 10000;

TEST(recompose_decompose_2d) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto M0 = rand_pos(100);
    auto M1 = rand_pos(100);
    auto x = rand_bounded(M0 * M1);
    CHECK(prop_recompose_decompose_2d(M0, M1, x));
  }
}

TEST(decompose_recompose_2d) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto M0 = rand_pos(100);
    auto M1 = rand_pos(100);
    auto x0 = rand_bounded(M0);
    auto x1 = rand_bounded(M1);
    CHECK(prop_decompose_recompose_2d(M0, M1, x0, x1));
  }
}

TEST(coalesce_preserves_function) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto s0 = rand_pos(50);
    auto d0 = rand_pos(50);
    auto s1 = rand_pos(50);
    auto d1 = s0 * d0;
    auto x0 = rand_bounded(s0);
    auto x1 = rand_bounded(s1);
    CHECK(prop_coalesce_preserves_function(s0, d0, s1, d1, x0, x1));
  }
}

TEST(ceilDiv_le_iff) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    auto Q = rand_nat();
    CHECK(prop_ceilDiv_le_iff(a, b, Q));
  }
}

TEST(ceilDiv_assoc) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto m = rand_pos();
    auto n = rand_pos();
    auto val = rand_nat();
    CHECK(prop_ceilDiv_assoc(val, m, n));
  }
}

TEST(ceilDiv_of_dvd) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto d = rand_pos();
    auto k = rand_nat(100);
    auto n = k * d;
    CHECK(prop_ceilDiv_of_dvd(n, d));
  }
}

TEST(ceilDiv_eq_div_add_one_of_not_dvd) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto d = rand_pos(100) + 1; // at least 2
    auto n = rand_nat();
    if (n % d == 0)
      n++; // ensure indivisible
    if (d > 1) {
      CHECK(prop_ceilDiv_eq_div_add_one_of_not_dvd(n, d));
    }
  }
}

TEST(ceilDiv_mul_ge_self) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    CHECK(prop_ceilDiv_mul_ge_self(a, b));
  }
}

TEST(ceilDiv_mul_sub_self_pos_of_not_dvd) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto d = rand_pos(100) + 1;
    auto n = rand_nat();
    if (n % d == 0)
      n++;
    if (d > 1 && n % d != 0) {
      CHECK(prop_ceilDiv_mul_sub_self_pos_of_not_dvd(n, d));
    }
  }
}

TEST(ceilDiv_eq_zero_iff) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    CHECK(prop_ceilDiv_eq_zero_iff(a, b));
  }
}

TEST(ceilDiv_mono_left) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto delta = rand_nat();
    auto b = rand_pos();
    CHECK(prop_ceilDiv_mono_left(a, a + delta, b));
  }
}

TEST(ceilDiv_antitone_right) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    auto delta = rand_nat();
    CHECK(prop_ceilDiv_antitone_right(a, b, b + delta));
  }
}

TEST(ceilDiv_mul_sub_self_eq_zero_iff) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto n = rand_nat();
    auto d = rand_pos();
    CHECK(prop_ceilDiv_mul_sub_self_eq_zero_iff(n, d));
  }
}

TEST(fttc) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto e = rand_pos();
    auto B = rand_pos();
    auto S = rand_pos();
    CHECK(prop_fttc(e, B, S));
  }
}

TEST(fttc_figure5) {
  // Figure 5 from tma-modeling-in-depth.md: e=3, B=5, S=8
  bool violated = (3 < 5) && (5 < 8) && (5 % 3 != 0);
  CHECK(violated); // Should be violated
  CHECK(prop_fttc(3, 5, 8));
}

TEST(thm_2_5) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_pos();
    auto r = rand_bounded(a);
    CHECK(prop_thm_2_5(r, a));
  }
}

TEST(thm_2_7_1) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto c = rand_pos();
    auto k = rand_nat(100);
    auto a = k * c;
    auto b = rand_nat();
    CHECK(prop_thm_2_7_1(a, b, c));
  }
}

TEST(thm_2_7_2) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    auto c = rand_pos();
    CHECK(prop_thm_2_7_2(a, b, c));
  }
}

TEST(thm_2_10) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto c = rand_pos();
    auto k = rand_nat(100);
    auto b = k * c;
    auto a = rand_nat(100); // smaller to avoid overflow
    CHECK(prop_thm_2_10(a, b, c));
  }
}

TEST(thm_2_11) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos();
    auto c = rand_pos();
    CHECK(prop_thm_2_11(a, b, c));
  }
}

TEST(thm_2_12) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos(100); // smaller to avoid overflow
    auto c = rand_pos(100);
    CHECK(prop_thm_2_12(a, b, c));
  }
}

TEST(thm_2_15_1) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto a = rand_nat();
    auto b = rand_pos(100);
    auto c = rand_pos(100);
    CHECK(prop_thm_2_15_1(a, b, c));
  }
}

TEST(thm_2_16) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto d = rand_pos();
    auto D = rand_nat(100);
    auto val = rand_nat();
    CHECK(prop_thm_2_16(val, D, d));
  }
}

TEST(merge_split_identity) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto extent = rand_pos();
    auto factor = rand_pos();
    CHECK(prop_merge_split_identity(extent, factor));
  }
}

TEST(split_split_extent) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto m = rand_pos(100);
    auto n = rand_pos(100);
    auto val = rand_nat();
    CHECK(prop_split_split_extent(m, n, val));
  }
}

TEST(split_split_outer) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto m = rand_pos(100);
    auto n = rand_pos(100);
    auto val = rand_nat();
    CHECK(prop_split_split_outer(m, n, val));
  }
}

TEST(split_split_inner_outer) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto m = rand_pos(100);
    auto n = rand_pos(100);
    auto val = rand_nat();
    CHECK(prop_split_split_inner_outer(m, n, val));
  }
}

TEST(split_split_inner_inner) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto m = rand_pos(100);
    auto n = rand_pos(100);
    auto val = rand_nat();
    CHECK(prop_split_split_inner_inner(m, n, val));
  }
}

TEST(predication_thm_2) {
  for (size_t i = 0; i < ITERATIONS; ++i) {
    auto N0 = rand_pos();
    auto N1 = rand_pos();
    auto i2 = rand_nat();
    CHECK(prop_predication_thm_2(i2, N0, N1));
  }
}

// ============================================================================
// Main
// ============================================================================

int main() {
  printf("\n");
  printf(
      "╔══════════════════════════════════════════════════════════════════╗\n");
  printf("║  Villa Straylight Property Tests                                  "
         "║\n");
  printf("║  26 theorems from Lean, validated in C++                          "
         "║\n");
  printf(
      "╚══════════════════════════════════════════════════════════════════╝\n");
  printf("\n");

  init_rng();

  SECTION("Coordinate Isomorphism");
  SECTION("Coalescence");
  SECTION("Ceiling Division");
  SECTION("FTTC");
  SECTION("Integer Division");
  SECTION("Merge-Split");
  SECTION("Split-Split");
  SECTION("Predication");

  printf("\n");
  printf(
      "════════════════════════════════════════════════════════════════════\n");
  printf("  Total: %zu checks, %zu passed, %zu failed\n", tests_run,
         tests_passed, tests_failed);
  printf(
      "════════════════════════════════════════════════════════════════════\n");

  if (tests_failed > 0) {
    printf("\n\033[31mFAILED\033[0m\n");
    return 1;
  } else {
    printf("\n\033[32mAll Villa Straylight theorems validated.\033[0m\n");
    printf("\"NVIDIA gave us the theorems. We gave them types.\"\n\n");
    return 0;
  }
}
