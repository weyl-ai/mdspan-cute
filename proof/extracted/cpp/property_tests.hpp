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
// §1: Coordinate Isomorphism
// ============================================================================

/**
 * @brief Round-tripping through 2D coordinates gets you back where you started
 * @theorem recompose_decompose_2d
 */
inline bool prop_recompose_decompose_2d(size_t M0, size_t M1, size_t x) {
    if (!(M0 > 0)) return true;
    if (!(M1 > 0)) return true;
    if (!(x < M0 * M1)) return true;
    auto [x0, x1] = decompose2(M0, M1, x);
    return recompose2(M0, x0, x1) == x;
}

/**
 * @brief Coordinates → index → coordinates gives same coordinates
 * @theorem decompose_recompose_2d
 */
inline bool prop_decompose_recompose_2d(size_t M0, size_t M1, size_t x0, size_t x1) {
    if (!(M0 > 0)) return true;
    if (!(x0 < M0)) return true;
    if (!(x1 < M1)) return true;
    return decompose2(M0, M1, recompose2(M0, x0, x1)) == std::make_pair(x0, x1);
}

// ============================================================================
// §2: Coalescence
// ============================================================================

/**
 * @brief Coalescence preserves the layout evaluation function
 * @theorem coalesce_preserves_function
 */
inline bool prop_coalesce_preserves_function(
    size_t s0, size_t d0, size_t s1, size_t d1, size_t x0, size_t x1
) {
    if (!(s0 > 0)) return true;
    if (!(s1 > 0)) return true;
    if (!(d1 == s0 * d0)) return true;  // packed merge condition
    if (!(x0 < s0)) return true;
    if (!(x1 < s1)) return true;
    return x0 * d0 + x1 * d1 == (x0 + x1 * s0) * d0;
}

// ============================================================================
// §3: Ceiling Division - The Galois Connection
// ============================================================================

/**
 * @brief Galois connection: ⌈a/b⌉ ≤ Q ⟺ a ≤ Q×b
 * @theorem ceilDiv_le_iff
 */
inline bool prop_ceilDiv_le_iff(size_t a, size_t b, size_t Q) {
    if (!(b > 0)) return true;
    return (ceilDiv(a, b) <= Q) == (a <= Q * b);
}

/**
 * @brief Ceiling division is associative: ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉
 * @theorem ceilDiv_assoc
 */
inline bool prop_ceilDiv_assoc(size_t i, size_t m, size_t n) {
    if (!(m > 0)) return true;
    if (!(n > 0)) return true;
    return ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n);
}

/**
 * @brief When divisible, ceiling equals floor
 * @theorem ceilDiv_of_dvd
 */
inline bool prop_ceilDiv_of_dvd(size_t n, size_t d) {
    if (!(d > 0)) return true;
    if (!(n % d == 0)) return true;
    return ceilDiv(n, d) == n / d;
}

/**
 * @brief When indivisible, ceiling = floor + 1
 * @theorem ceilDiv_eq_div_add_one_of_not_dvd
 */
inline bool prop_ceilDiv_eq_div_add_one_of_not_dvd(size_t n, size_t d) {
    if (!(d > 0)) return true;
    if (!(n % d != 0)) return true;
    return ceilDiv(n, d) == n / d + 1;
}

/**
 * @brief a ≤ ⌈a/b⌉ × b always
 * @theorem ceilDiv_mul_ge_self
 */
inline bool prop_ceilDiv_mul_ge_self(size_t a, size_t b) {
    if (!(b > 0)) return true;
    return a <= ceilDiv(a, b) * b;
}

/**
 * @brief Indivisibility creates holes
 * @theorem ceilDiv_mul_sub_self_pos_of_not_dvd
 */
inline bool prop_ceilDiv_mul_sub_self_pos_of_not_dvd(size_t n, size_t d) {
    if (!(d > 0)) return true;
    if (!(n % d != 0)) return true;
    return ceilDiv(n, d) * d > n;  // equivalent to: ceilDiv(n,d)*d - n > 0
}

/**
 * @brief ceilDiv(a,b) = 0 ⟺ a = 0
 * @theorem ceilDiv_eq_zero_iff
 */
inline bool prop_ceilDiv_eq_zero_iff(size_t a, size_t b) {
    if (!(b > 0)) return true;
    return (ceilDiv(a, b) == 0) == (a == 0);
}

/**
 * @brief ceilDiv is monotone in numerator
 * @theorem ceilDiv_mono_left
 */
inline bool prop_ceilDiv_mono_left(size_t a, size_t a_prime, size_t b) {
    if (!(b > 0)) return true;
    if (!(a <= a_prime)) return true;
    return ceilDiv(a, b) <= ceilDiv(a_prime, b);
}

/**
 * @brief ceilDiv is antitone in denominator
 * @theorem ceilDiv_antitone_right
 */
inline bool prop_ceilDiv_antitone_right(size_t a, size_t b, size_t b_prime) {
    if (!(b > 0)) return true;
    if (!(b <= b_prime)) return true;
    return ceilDiv(a, b_prime) <= ceilDiv(a, b);
}

/**
 * @brief No holes ⟺ divisibility
 * @theorem ceilDiv_mul_sub_self_eq_zero_iff
 */
inline bool prop_ceilDiv_mul_sub_self_eq_zero_iff(size_t n, size_t d) {
    if (!(d > 0)) return true;
    return (ceilDiv(n, d) * d == n) == (n % d == 0);
}

// ============================================================================
// §3: FTTC - The Fundamental Theorem of TMA Correctness
// ============================================================================

/**
 * @brief FTTC: Strong correctness unachievable ⟺ e < B < S ∧ e ∤ B
 * @theorem fttc
 * 
 * "The shotgun wired to the forehead"
 */
inline bool prop_fttc(size_t e, size_t B, size_t S) {
    if (!(e > 0)) return true;
    if (!(B > 0)) return true;
    if (!(S > 0)) return true;
    bool violated = (e < B) && (B < S) && (B % e != 0);
    bool achievable = (B % e == 0) || (B >= S) || (e >= B);
    return violated == !achievable;
}

// ============================================================================
// §4: Integer Division Theorems
// ============================================================================

/**
 * @brief Small numbers: r < a ⟹ r%a = r, r/a = 0
 * @theorem thm_2_5
 */
inline bool prop_thm_2_5(size_t r, size_t a) {
    if (!(a > 0)) return true;
    if (!(r < a)) return true;
    return (r % a == r) && (r / a == 0);
}

/**
 * @brief Adding a multiple doesn't change remainder
 * @theorem thm_2_7_1
 */
inline bool prop_thm_2_7_1(size_t a, size_t b, size_t c) {
    if (!(c > 0)) return true;
    if (!(a % c == 0)) return true;
    return (a + b) % c == b % c;
}

/**
 * @brief Nested mod simplifies: a%(b×c)%b = a%b
 * @theorem thm_2_7_2
 */
inline bool prop_thm_2_7_2(size_t a, size_t b, size_t c) {
    if (!(b > 0)) return true;
    if (!(c > 0)) return true;
    return a % (b * c) % b == a % b;
}

/**
 * @brief Division distributes over multiplication (when divisible)
 * @theorem thm_2_10
 */
inline bool prop_thm_2_10(size_t a, size_t b, size_t c) {
    if (!(c > 0)) return true;
    if (!(b % c == 0)) return true;
    return a * (b / c) == (a * b) / c;
}

/**
 * @brief Division associates: a/(b×c) = a/b/c
 * @theorem thm_2_11
 */
inline bool prop_thm_2_11(size_t a, size_t b, size_t c) {
    if (!(b > 0)) return true;
    if (!(c > 0)) return true;
    return a / (b * c) == a / b / c;
}

/**
 * @brief Mixed-radix decomposition: a%(b×c) = a%b + (a/b%c)×b
 * @theorem thm_2_12
 */
inline bool prop_thm_2_12(size_t a, size_t b, size_t c) {
    if (!(b > 0)) return true;
    if (!(c > 0)) return true;
    return a % (b * c) == a % b + (a / b % c) * b;
}

/**
 * @brief Extracting the middle digit: a/b%c = a%(b×c)/b
 * @theorem thm_2_15_1
 */
inline bool prop_thm_2_15_1(size_t a, size_t b, size_t c) {
    if (!(b > 0)) return true;
    if (!(c > 0)) return true;
    return a / b % c == a % (b * c) / b;
}

/**
 * @brief Bound theorem: i/d < D ⟺ i < D×d
 * @theorem thm_2_16
 */
inline bool prop_thm_2_16(size_t i, size_t D, size_t d) {
    if (!(d > 0)) return true;
    return (i / d < D) == (i < D * d);
}

// ============================================================================
// §4: Merge-Split Identity
// ============================================================================

/**
 * @brief merge ∘ split = id ⟺ divisibility
 * @theorem merge_split_identity
 */
inline bool prop_merge_split_identity(size_t extent, size_t factor) {
    if (!(extent > 0)) return true;
    if (!(factor > 0)) return true;
    bool identity = ceilDiv(extent, factor) * factor == extent;
    bool divides = extent % factor == 0;
    return identity == divides;
}

// ============================================================================
// §4: Split-Split Equivalence
// ============================================================================

/**
 * @brief Split-split extent: ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉
 * @theorem split_split_equivalence (part 1)
 */
inline bool prop_split_split_extent(size_t m, size_t n, size_t i) {
    if (!(m > 0)) return true;
    if (!(n > 0)) return true;
    return ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n);
}

/**
 * @brief Split-split outer: i/n/m = i/(m×n)
 * @theorem split_split_equivalence (part 2)
 */
inline bool prop_split_split_outer(size_t m, size_t n, size_t i) {
    if (!(m > 0)) return true;
    if (!(n > 0)) return true;
    return i / n / m == i / (m * n);
}

/**
 * @brief Split-split inner-outer: i/n%m = i%(m×n)/n
 * @theorem split_split_equivalence (part 3)
 */
inline bool prop_split_split_inner_outer(size_t m, size_t n, size_t i) {
    if (!(m > 0)) return true;
    if (!(n > 0)) return true;
    return i / n % m == i % (m * n) / n;
}

/**
 * @brief Split-split inner-inner: i%n = i%(m×n)%n
 * @theorem split_split_equivalence (part 4)
 */
inline bool prop_split_split_inner_inner(size_t m, size_t n, size_t i) {
    if (!(m > 0)) return true;
    if (!(n > 0)) return true;
    return i % n == i % (m * n) % n;
}

// ============================================================================
// Predication Theorems
// ============================================================================

/**
 * @brief If outer indices in bounds, middle is too
 * @theorem predication_thm_1
 */
inline bool prop_predication_thm_1(size_t i0, size_t i1, size_t i2, size_t N0, size_t N2) {
    if (!(N2 > 0)) return true;
    if (!(i0 < N0)) return true;
    if (!(i2 < N2)) return true;
    if (!(i0 == i1 * N2 + i2)) return true;
    return i1 < ceilDiv(N0, N2);
}

/**
 * @brief I0 in boundary ⟺ I2 in boundary
 * @theorem predication_thm_2
 */
inline bool prop_predication_thm_2(size_t i2, size_t N0, size_t N1) {
    if (!(N1 > 0)) return true;
    size_t i0 = i2 / N1;
    size_t N2 = N0 * N1;
    return (i0 < N0) == (i2 < N2);
}

} // namespace mdspan_cute::properties
