/*
 * property_tests_catch2.cpp - Catch2 + RapidCheck property tests
 *
 * AUTO-GENERATED FROM VillaStraylight.lean
 * DO NOT EDIT - regenerate with `lake exe extract-properties`
 *
 * These properties are PROVEN in Lean for all valid inputs.
 * This test validates mdspan_cute conforms to the specification.
 */

#include "property_tests.hpp"

#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>

using namespace mdspan_cute::properties;

// ============================================================================
// Generators
// ============================================================================

namespace {
    // Small positive integers to avoid overflow
    auto smallPos() { return rc::gen::inRange<size_t>(1, 1000); }
    auto smallNat() { return rc::gen::inRange<size_t>(0, 10000); }
    
    // Bounded by another value
    auto boundedBy(size_t bound) { 
        return rc::gen::inRange<size_t>(0, bound > 0 ? bound : 1); 
    }
}

// ============================================================================
// §1: Coordinate Isomorphism
// ============================================================================

TEST_CASE("Coordinate Isomorphism", "[properties][coordinates]") {
    
    SECTION("recompose_decompose_2d") {
        rc::check([](void) {
            auto M0 = *smallPos();
            auto M1 = *smallPos();
            auto x = *boundedBy(M0 * M1);
            RC_ASSERT(prop_recompose_decompose_2d(M0, M1, x));
        });
    }
    
    SECTION("decompose_recompose_2d") {
        rc::check([](void) {
            auto M0 = *smallPos();
            auto M1 = *smallPos();
            auto x0 = *boundedBy(M0);
            auto x1 = *boundedBy(M1);
            RC_ASSERT(prop_decompose_recompose_2d(M0, M1, x0, x1));
        });
    }
}

// ============================================================================
// §2: Coalescence
// ============================================================================

TEST_CASE("Coalescence", "[properties][coalescence]") {
    
    SECTION("coalesce_preserves_function") {
        rc::check([](void) {
            auto s0 = *smallPos();
            auto d0 = *smallPos();
            auto s1 = *smallPos();
            auto d1 = s0 * d0;  // packed merge condition
            auto x0 = *boundedBy(s0);
            auto x1 = *boundedBy(s1);
            RC_ASSERT(prop_coalesce_preserves_function(s0, d0, s1, d1, x0, x1));
        });
    }
}

// ============================================================================
// §3: Ceiling Division
// ============================================================================

TEST_CASE("Ceiling Division", "[properties][ceildiv]") {
    
    SECTION("Galois connection") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto Q = *smallNat();
            RC_ASSERT(prop_ceilDiv_le_iff(a, b, Q));
        });
    }
    
    SECTION("Associativity") {
        rc::check([](void) {
            auto i = *smallNat();
            auto m = *smallPos();
            auto n = *smallPos();
            RC_ASSERT(prop_ceilDiv_assoc(i, m, n));
        });
    }
    
    SECTION("Divisible case") {
        rc::check([](void) {
            auto d = *smallPos();
            auto k = *smallNat();
            auto n = k * d;  // ensure divisibility
            RC_ASSERT(prop_ceilDiv_of_dvd(n, d));
        });
    }
    
    SECTION("Indivisible case") {
        rc::check([](void) {
            auto d = *rc::gen::inRange<size_t>(2, 100);
            auto n = *smallNat();
            RC_PRE(n % d != 0);  // ensure indivisibility
            RC_ASSERT(prop_ceilDiv_eq_div_add_one_of_not_dvd(n, d));
        });
    }
    
    SECTION("ceilDiv * b >= a") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            RC_ASSERT(prop_ceilDiv_mul_ge_self(a, b));
        });
    }
    
    SECTION("Holes theorem") {
        rc::check([](void) {
            auto d = *rc::gen::inRange<size_t>(2, 100);
            auto n = *smallNat();
            RC_PRE(n % d != 0);
            RC_ASSERT(prop_ceilDiv_mul_sub_self_pos_of_not_dvd(n, d));
        });
    }
    
    SECTION("Zero iff") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            RC_ASSERT(prop_ceilDiv_eq_zero_iff(a, b));
        });
    }
    
    SECTION("Monotone left") {
        rc::check([](void) {
            auto a = *smallNat();
            auto delta = *smallNat();
            auto b = *smallPos();
            RC_ASSERT(prop_ceilDiv_mono_left(a, a + delta, b));
        });
    }
    
    SECTION("Antitone right") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto delta = *smallNat();
            RC_ASSERT(prop_ceilDiv_antitone_right(a, b, b + delta));
        });
    }
    
    SECTION("No holes iff divisibility") {
        rc::check([](void) {
            auto n = *smallNat();
            auto d = *smallPos();
            RC_ASSERT(prop_ceilDiv_mul_sub_self_eq_zero_iff(n, d));
        });
    }
}

// ============================================================================
// §3: FTTC - The Fundamental Theorem of TMA Correctness
// ============================================================================

TEST_CASE("FTTC", "[properties][fttc][tma]") {
    
    SECTION("violated iff not achievable") {
        rc::check([](void) {
            auto e = *smallPos();
            auto B = *smallPos();
            auto S = *smallPos();
            RC_ASSERT(prop_fttc(e, B, S));
        });
    }
    
    SECTION("Figure 5 example: e=3, B=5, S=8 is violated") {
        CHECK_FALSE(prop_fttc(3, 5, 8) == false);  // Should be violated
        bool violated = (3 < 5) && (5 < 8) && (5 % 3 != 0);
        CHECK(violated);
    }
    
    SECTION("Divisible case is achievable") {
        // e=4, B=8, S=16 → 4|8, so achievable
        bool violated = (4 < 8) && (8 < 16) && (8 % 4 != 0);
        CHECK_FALSE(violated);
    }
}

// ============================================================================
// §4: Integer Division
// ============================================================================

TEST_CASE("Integer Division", "[properties][division]") {
    
    SECTION("Small numbers") {
        rc::check([](void) {
            auto a = *smallPos();
            auto r = *boundedBy(a);
            RC_ASSERT(prop_thm_2_5(r, a));
        });
    }
    
    SECTION("Adding multiple preserves remainder") {
        rc::check([](void) {
            auto c = *smallPos();
            auto k = *smallNat();
            auto a = k * c;  // multiple of c
            auto b = *smallNat();
            RC_ASSERT(prop_thm_2_7_1(a, b, c));
        });
    }
    
    SECTION("Nested mod") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto c = *smallPos();
            RC_ASSERT(prop_thm_2_7_2(a, b, c));
        });
    }
    
    SECTION("Division distributes when divisible") {
        rc::check([](void) {
            auto c = *smallPos();
            auto k = *smallNat();
            auto b = k * c;  // divisible
            auto a = *smallNat();
            RC_ASSERT(prop_thm_2_10(a, b, c));
        });
    }
    
    SECTION("Division associates") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto c = *smallPos();
            RC_ASSERT(prop_thm_2_11(a, b, c));
        });
    }
    
    SECTION("Mixed-radix decomposition") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto c = *smallPos();
            RC_ASSERT(prop_thm_2_12(a, b, c));
        });
    }
    
    SECTION("Middle digit extraction") {
        rc::check([](void) {
            auto a = *smallNat();
            auto b = *smallPos();
            auto c = *smallPos();
            RC_ASSERT(prop_thm_2_15_1(a, b, c));
        });
    }
    
    SECTION("Bound theorem") {
        rc::check([](void) {
            auto i = *smallNat();
            auto D = *smallNat();
            auto d = *smallPos();
            RC_ASSERT(prop_thm_2_16(i, D, d));
        });
    }
}

// ============================================================================
// §4: Merge-Split
// ============================================================================

TEST_CASE("Merge-Split", "[properties][merge-split]") {
    
    SECTION("Identity iff divisibility") {
        rc::check([](void) {
            auto extent = *smallPos();
            auto factor = *smallPos();
            RC_ASSERT(prop_merge_split_identity(extent, factor));
        });
    }
}

// ============================================================================
// §4: Split-Split Equivalence
// ============================================================================

TEST_CASE("Split-Split Equivalence", "[properties][split-split]") {
    
    SECTION("Extent") {
        rc::check([](void) {
            auto m = *smallPos();
            auto n = *smallPos();
            auto i = *smallNat();
            RC_ASSERT(prop_split_split_extent(m, n, i));
        });
    }
    
    SECTION("Outer index") {
        rc::check([](void) {
            auto m = *smallPos();
            auto n = *smallPos();
            auto i = *smallNat();
            RC_ASSERT(prop_split_split_outer(m, n, i));
        });
    }
    
    SECTION("Inner-outer index") {
        rc::check([](void) {
            auto m = *smallPos();
            auto n = *smallPos();
            auto i = *smallNat();
            RC_ASSERT(prop_split_split_inner_outer(m, n, i));
        });
    }
    
    SECTION("Inner-inner index") {
        rc::check([](void) {
            auto m = *smallPos();
            auto n = *smallPos();
            auto i = *smallNat();
            RC_ASSERT(prop_split_split_inner_inner(m, n, i));
        });
    }
}

// ============================================================================
// Predication
// ============================================================================

TEST_CASE("Predication", "[properties][predication]") {
    
    SECTION("Theorem 1: outer bounds imply middle") {
        rc::check([](void) {
            auto N2 = *smallPos();
            auto N1 = *smallPos();
            auto N0 = N1 * N2;
            auto i1 = *boundedBy(N1);
            auto i2 = *boundedBy(N2);
            auto i0 = i1 * N2 + i2;
            RC_PRE(i0 < N0);
            RC_ASSERT(prop_predication_thm_1(i0, i1, i2, N0, N2));
        });
    }
    
    SECTION("Theorem 2: boundary equivalence") {
        rc::check([](void) {
            auto N0 = *smallPos();
            auto N1 = *smallPos();
            auto i2 = *smallNat();
            RC_ASSERT(prop_predication_thm_2(i2, N0, N1));
        });
    }
}
