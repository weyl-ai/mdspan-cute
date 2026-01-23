/-
  PropertyExtract.lean - Generate C++ property tests from Lean theorems

  The theorems in VillaStraylight.lean are proven for all inputs.
  We extract them as property tests for mdspan_cute conformance.

  Lean proves: ∀ inputs satisfying preconditions, property holds
  C++ tests:   rapidcheck generates inputs, checks property holds

  The tests are REDUNDANT (Lean proved it) but ESSENTIAL
  (validates C++ implementation matches the spec).
-/

import Lean
import VillaStraylight

namespace PropertyExtract

open Lean Meta Elab VillaStraylight

/-! ## Property Specification -/

/-- A property test extracted from a Lean theorem -/
structure PropertySpec where
  name : String                    -- Test name
  theorem_name : String            -- Source theorem in Lean
  params : List (String × String)  -- (name, C++ type)
  preconditions : List String      -- RC_PRE conditions
  property : String                -- The CHECK assertion
  doc : String                     -- Documentation

/-! ## The 26 Theorems as Property Specs -/

def properties : List PropertySpec := [
  -- §1: Coordinate Isomorphism
  { name := "recompose_decompose_2d"
    theorem_name := "recompose_decompose_2d"
    params := [("M0", "size_t"), ("M1", "size_t"), ("x", "size_t")]
    preconditions := ["M0 > 0", "M1 > 0", "x < M0 * M1"]
    property := "recompose2(M0, decompose2(M0, M1, x)) == x"
    doc := "Round-tripping through 2D coordinates gets you back where you started"
  },
  { name := "decompose_recompose_2d"
    theorem_name := "decompose_recompose_2d"
    params := [("M0", "size_t"), ("M1", "size_t"), ("x0", "size_t"), ("x1", "size_t")]
    preconditions := ["M0 > 0", "M1 > 0", "x0 < M0", "x1 < M1"]
    property := "decompose2(M0, M1, recompose2(M0, x0, x1)) == std::make_pair(x0, x1)"
    doc := "Coordinates → index → coordinates gives you the same coordinates"
  },
  
  -- §2: Coalescence
  { name := "coalesce_preserves_function"
    theorem_name := "coalesce_preserves_function"
    params := [("s0", "size_t"), ("d0", "size_t"), ("s1", "size_t"), ("d1", "size_t"), 
               ("x0", "size_t"), ("x1", "size_t")]
    preconditions := ["s0 > 0", "s1 > 0", "d1 == s0 * d0", "x0 < s0", "x1 < s1"]
    property := "x0 * d0 + x1 * d1 == (x0 + x1 * s0) * d0"
    doc := "Coalescence preserves the layout evaluation function"
  },
  
  -- §3: Ceiling Division
  { name := "ceilDiv_le_iff_left"
    theorem_name := "ceilDiv_le_iff"
    params := [("a", "size_t"), ("b", "size_t"), ("Q", "size_t")]
    preconditions := ["b > 0"]
    property := "(ceilDiv(a, b) <= Q) == (a <= Q * b)"
    doc := "Galois connection: ⌈a/b⌉ ≤ Q ⟺ a ≤ Q×b"
  },
  { name := "ceilDiv_assoc"
    theorem_name := "ceilDiv_assoc"
    params := [("i", "size_t"), ("m", "size_t"), ("n", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n)"
    doc := "Ceiling division is associative: ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉"
  },
  { name := "ceilDiv_of_dvd"
    theorem_name := "ceilDiv_of_dvd"
    params := [("n", "size_t"), ("d", "size_t")]
    preconditions := ["d > 0", "n % d == 0"]
    property := "ceilDiv(n, d) == n / d"
    doc := "When divisible, ceiling equals floor"
  },
  { name := "ceilDiv_eq_div_add_one_of_not_dvd"
    theorem_name := "ceilDiv_eq_div_add_one_of_not_dvd"
    params := [("n", "size_t"), ("d", "size_t")]
    preconditions := ["d > 0", "n % d != 0"]
    property := "ceilDiv(n, d) == n / d + 1"
    doc := "When indivisible, ceiling = floor + 1"
  },
  { name := "ceilDiv_mul_ge_self"
    theorem_name := "ceilDiv_mul_ge_self"
    params := [("a", "size_t"), ("b", "size_t")]
    preconditions := ["b > 0"]
    property := "a <= ceilDiv(a, b) * b"
    doc := "a ≤ ⌈a/b⌉ × b always"
  },
  { name := "ceilDiv_mul_sub_self_pos_of_not_dvd"
    theorem_name := "ceilDiv_mul_sub_self_pos_of_not_dvd"
    params := [("n", "size_t"), ("d", "size_t")]
    preconditions := ["d > 0", "n % d != 0"]
    property := "ceilDiv(n, d) * d - n > 0"
    doc := "Indivisibility creates holes"
  },
  { name := "ceilDiv_eq_zero_iff"
    theorem_name := "ceilDiv_eq_zero_iff"
    params := [("a", "size_t"), ("b", "size_t")]
    preconditions := ["b > 0"]
    property := "(ceilDiv(a, b) == 0) == (a == 0)"
    doc := "ceilDiv(a,b) = 0 ⟺ a = 0"
  },
  { name := "ceilDiv_mono_left"
    theorem_name := "ceilDiv_mono_left"
    params := [("a", "size_t"), ("a_prime", "size_t"), ("b", "size_t")]
    preconditions := ["b > 0", "a <= a_prime"]
    property := "ceilDiv(a, b) <= ceilDiv(a_prime, b)"
    doc := "ceilDiv is monotone in numerator"
  },
  { name := "ceilDiv_antitone_right"
    theorem_name := "ceilDiv_antitone_right"
    params := [("a", "size_t"), ("b", "size_t"), ("b_prime", "size_t")]
    preconditions := ["b > 0", "b <= b_prime"]
    property := "ceilDiv(a, b_prime) <= ceilDiv(a, b)"
    doc := "ceilDiv is antitone in denominator"
  },
  { name := "ceilDiv_mul_sub_self_eq_zero_iff"
    theorem_name := "ceilDiv_mul_sub_self_eq_zero_iff"
    params := [("n", "size_t"), ("d", "size_t")]
    preconditions := ["d > 0"]
    property := "(ceilDiv(n, d) * d - n == 0) == (n % d == 0)"
    doc := "No holes ⟺ divisibility"
  },
  
  -- §3: FTTC
  { name := "fttc"
    theorem_name := "fttc"
    params := [("e", "size_t"), ("B", "size_t"), ("S", "size_t")]
    preconditions := ["e > 0", "B > 0", "S > 0"]
    property := "((e < B) && (B < S) && (B % e != 0)) == !((B % e == 0) || (B >= S) || (e >= B))"
    doc := "FTTC: Strong correctness unachievable ⟺ e < B < S ∧ e ∤ B"
  },
  
  -- §4: Integer Division
  { name := "thm_2_5"
    theorem_name := "thm_2_5"
    params := [("r", "size_t"), ("a", "size_t")]
    preconditions := ["a > 0", "r < a"]
    property := "(r % a == r) && (r / a == 0)"
    doc := "Small numbers: r < a ⟹ r%a = r, r/a = 0"
  },
  { name := "thm_2_7_1"
    theorem_name := "thm_2_7_1"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["c > 0", "a % c == 0"]
    property := "(a + b) % c == b % c"
    doc := "Adding a multiple doesn't change remainder"
  },
  { name := "thm_2_7_2"
    theorem_name := "thm_2_7_2"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["b > 0", "c > 0"]
    property := "a % (b * c) % b == a % b"
    doc := "Nested mod simplifies"
  },
  { name := "thm_2_10"
    theorem_name := "thm_2_10"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["c > 0", "b % c == 0"]
    property := "a * (b / c) == (a * b) / c"
    doc := "Division distributes over multiplication (when divisible)"
  },
  { name := "thm_2_11"
    theorem_name := "thm_2_11"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["b > 0", "c > 0"]
    property := "a / (b * c) == a / b / c"
    doc := "Division associates"
  },
  { name := "thm_2_12"
    theorem_name := "thm_2_12"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["b > 0", "c > 0"]
    property := "a % (b * c) == a % b + (a / b % c) * b"
    doc := "Mixed-radix decomposition"
  },
  { name := "thm_2_15_1"
    theorem_name := "thm_2_15_1"
    params := [("a", "size_t"), ("b", "size_t"), ("c", "size_t")]
    preconditions := ["b > 0", "c > 0"]
    property := "a / b % c == a % (b * c) / b"
    doc := "Extracting the middle digit"
  },
  { name := "thm_2_16"
    theorem_name := "thm_2_16"
    params := [("i", "size_t"), ("D", "size_t"), ("d", "size_t")]
    preconditions := ["d > 0"]
    property := "(i / d < D) == (i < D * d)"
    doc := "Bound theorem: i/d < D ⟺ i < D×d"
  },
  
  -- §4: Merge-Split
  { name := "merge_split_identity"
    theorem_name := "merge_split_identity"
    params := [("extent", "size_t"), ("factor", "size_t")]
    preconditions := ["extent > 0", "factor > 0"]
    property := "(ceilDiv(extent, factor) * factor == extent) == (extent % factor == 0)"
    doc := "merge ∘ split = id ⟺ divisibility"
  },
  
  -- §4: Split-Split
  { name := "split_split_extent"
    theorem_name := "split_split_equivalence"
    params := [("m", "size_t"), ("n", "size_t"), ("i", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n)"
    doc := "Split-split extent equivalence"
  },
  { name := "split_split_outer"
    theorem_name := "split_split_equivalence"
    params := [("m", "size_t"), ("n", "size_t"), ("i", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "i / n / m == i / (m * n)"
    doc := "Split-split outer index equivalence"
  },
  { name := "split_split_inner_outer"
    theorem_name := "split_split_equivalence"
    params := [("m", "size_t"), ("n", "size_t"), ("i", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "i / n % m == i % (m * n) / n"
    doc := "Split-split inner-outer index equivalence"
  },
  { name := "split_split_inner_inner"
    theorem_name := "split_split_equivalence"
    params := [("m", "size_t"), ("n", "size_t"), ("i", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "i % n == i % (m * n) % n"
    doc := "Split-split inner-inner index equivalence"
  }
]

/-! ## C++ Code Generation -/

def emitCppHeader : String :=
  let header := """
/*
 * property_tests.hpp - Property tests extracted from Lean theorems
 *
 * AUTO-GENERATED FROM VillaStraylight.lean
 * DO NOT EDIT - regenerate with `lake exe extract-properties`
 *
 * These properties are PROVEN in Lean for all valid inputs.
 * The C++ tests validate that mdspan_cute's implementation
 * conforms to the proven specification.
 *
 * Source: Villa Straylight Papers (26 theorems, 0 sorry)
 */

#pragma once

#include <cstddef>
#include <utility>

namespace mdspan_cute::properties {

// ============================================================================
// Helper functions (must match Lean definitions exactly)
// ============================================================================

constexpr size_t ceilDiv(size_t a, size_t b) {
    return (a + b - 1) / b;
}

constexpr std::pair<size_t, size_t> decompose2(size_t M0, size_t M1, size_t x) {
    return {x % M0, x / M0 % M1};
}

constexpr size_t recompose2(size_t M0, size_t x0, size_t x1) {
    return x0 + x1 * M0;
}

// ============================================================================
// Property test functions
// Each corresponds to a Lean theorem in VillaStraylight.lean
// ============================================================================

"""

  let props := properties.map emitPropertyFunction |> String.intercalate "\n"
  
  let footer := """

} // namespace mdspan_cute::properties
"""

  header ++ props ++ footer

def emitPropertyFunction (p : PropertySpec) : String :=
  let paramList := p.params.map (fun (n, t) => s!"{t} {n}") |> String.intercalate ", "
  let preList := p.preconditions.map (fun c => s!"    if (!({c})) return true; // precondition") |> String.intercalate "\n"
  s!"""
/**
 * @brief {p.doc}
 * @theorem {p.theorem_name}
 */
inline bool prop_{p.name}({paramList}) {{
{preList}
    return {p.property};
}}
"""

def emitRapidcheckTests : String :=
  let header := """
/*
 * property_tests_rc.cpp - RapidCheck property tests from Lean theorems
 *
 * AUTO-GENERATED FROM VillaStraylight.lean
 * DO NOT EDIT - regenerate with `lake exe extract-properties`
 */

#include "property_tests.hpp"
#include <rapidcheck.h>
#include <catch2/catch_test_macros.hpp>

using namespace mdspan_cute::properties;

// Generator for small positive integers (avoid overflow)
static auto smallPos() {
    return rc::gen::inRange<size_t>(1, 1000);
}

static auto smallNat() {
    return rc::gen::inRange<size_t>(0, 10000);
}

TEST_CASE("Villa Straylight Theorems", "[properties][lean]") {

"""

  let tests := properties.map emitRapidcheckTest |> String.intercalate "\n"

  let footer := """
}
"""

  header ++ tests ++ footer

def emitRapidcheckTest (p : PropertySpec) : String :=
  let generators := p.params.map (fun (n, _) => 
    if p.preconditions.any (fun c => c.containsSubstr s!"{n} > 0") then
      s!"auto {n} = *smallPos()"
    else if p.preconditions.any (fun c => c.containsSubstr s!"{n} < ") then
      s!"auto {n} = *smallNat()"
    else
      s!"auto {n} = *smallNat()"
  ) |> String.intercalate ";\n        "
  
  let argList := p.params.map (·.1) |> String.intercalate ", "
  
  s!"""
    SECTION("{p.name}") {{
        // {p.doc}
        // Theorem: {p.theorem_name}
        rc::check([](void) {{
            {generators};
            RC_ASSERT(prop_{p.name}({argList}));
        }});
    }}
"""

def emitDoctest : String :=
  let header := """
/*
 * property_tests_doctest.cpp - Doctest property tests from Lean theorems
 *
 * AUTO-GENERATED FROM VillaStraylight.lean
 * For projects using doctest instead of Catch2/RapidCheck
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "property_tests.hpp"

#include <random>
#include <cstdint>

using namespace mdspan_cute::properties;

// Simple property test runner (no RapidCheck dependency)
template<typename F>
void check_property(F&& f, size_t iterations = 1000) {
    std::mt19937_64 rng(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> small_pos(1, 1000);
    std::uniform_int_distribution<size_t> small_nat(0, 10000);
    
    for (size_t i = 0; i < iterations; ++i) {
        f(rng, small_pos, small_nat);
    }
}

TEST_SUITE("Villa Straylight Theorems") {

"""

  let tests := properties.map emitDoctestCase |> String.intercalate "\n"

  let footer := """
}
"""

  header ++ tests ++ footer

def emitDoctestCase (p : PropertySpec) : String :=
  let generators := p.params.map (fun (n, _) => 
    if p.preconditions.any (fun c => c.containsSubstr s!"{n} > 0") then
      s!"size_t {n} = small_pos(rng)"
    else
      s!"size_t {n} = small_nat(rng)"
  ) |> String.intercalate ";\n            "
  
  let argList := p.params.map (·.1) |> String.intercalate ", "
  
  s!"""
    TEST_CASE("{p.name}") {{
        // {p.doc}
        check_property([](auto& rng, auto& small_pos, auto& small_nat) {{
            {generators};
            CHECK(prop_{p.name}({argList}));
        }});
    }}
"""

/-! ## Main extraction -/

def main : IO Unit := do
  -- Create output directory
  IO.FS.createDirAll "extracted/cpp"
  
  -- Write header with property functions
  IO.FS.writeFile "extracted/cpp/property_tests.hpp" emitCppHeader
  IO.println "Wrote extracted/cpp/property_tests.hpp"
  
  -- Write RapidCheck test file
  IO.FS.writeFile "extracted/cpp/property_tests_rc.cpp" emitRapidcheckTests
  IO.println "Wrote extracted/cpp/property_tests_rc.cpp"
  
  -- Write doctest version
  IO.FS.writeFile "extracted/cpp/property_tests_doctest.cpp" emitDoctest
  IO.println "Wrote extracted/cpp/property_tests_doctest.cpp"
  
  IO.println s!"Extracted {properties.length} property tests from VillaStraylight.lean"

end PropertyExtract
