# Lean Proof System Architecture

## Overview

This directory contains a formal treatment of NVIDIA's layout algebra in Lean 4,
machine-verifying theorems that NVIDIA engineers originally proved by hand in
markdown documentation.

```
┌─────────────────────────────────────────────────────────────────┐
│                     NVIDIA Documentation                         │
│  (doc/math/integer-division.md, doc/reading/tma-modeling-*.md)  │
│              ~30 files of handwritten proofs in markdown         │
└────────────────────────────┬────────────────────────────────────┘
                             │ Encoded as
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   VillaStraylight.lean                           │
│              26 machine-verified theorems (0 sorry)              │
│                    Uses Mathlib 4.15.0                          │
└────────────────────────────┬────────────────────────────────────┘
                             │ Extracted via
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   PropertyExtract.lean                           │
│              Code generator for C++ property tests               │
└────────────────────────────┬────────────────────────────────────┘
                             │ Generates
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                 extracted/cpp/property_tests.hpp                 │
│                      + Catch2/RapidCheck tests                   │
│                Validates mdspan_cute implementation              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Project Structure

### Core Files

| File | Purpose |
|------|---------|
| `VillaStraylight.lean` | Main theorem file (1575 lines, 26 theorems) |
| `PropertyExtract.lean` | C++ property test code generator |
| `lakefile.lean` | Lake build configuration |
| `lean-toolchain` | Lean version: `leanprover/lean4:v4.15.0` |
| `lake-manifest.json` | Dependency manifest |
| `BUILD.md` | Build instructions |

### Generated Files

| File | Purpose |
|------|---------|
| `extracted/cpp/property_tests.hpp` | C++ header with property functions |
| `extracted/cpp/property_tests_catch2.cpp` | Catch2 + RapidCheck test suite |
| `extracted/cpp/property_tests_standalone.cpp` | Standalone test version |

### Documentation

| Directory | Content |
|-----------|---------|
| `doc/math/` | Mathematical foundations (integer division, algebra) |
| `doc/reading/` | GPU-specific theory (TMA, IterDomain, holes) |

---

## Build System

### Requirements

- Lean 4.15.0
- Lake (Lean package manager)
- Mathlib 4.15.0 (downloaded via cache)

### Build Commands

```bash
cd proof

# 1. Download pre-built Mathlib (REQUIRED - don't build from source)
lake exe cache get

# 2. Build
lake build
```

### Package Configuration

```lean
-- lakefile.lean
package «villa-straylight» where
  leanOptions := #[
    ⟨`pp.unicode.fun, true⟩,
    ⟨`autoImplicit, false⟩
  ]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "v4.15.0"
```

---

## Theorem Index

### §1: Coordinate Isomorphism (Mixed-Radix Arithmetic)

| Theorem | Statement | Source |
|---------|-----------|--------|
| `recompose_decompose_2d` | `recompose(decompose(x)) = x` | — |
| `decompose_recompose_2d` | `decompose(recompose(x₀,x₁)) = (x₀,x₁)` | — |
| `decompose2_bounds` | Decomposition stays in bounds | — |

### §2: Coalescence (Mode Merging)

| Theorem | Statement | Source |
|---------|-----------|--------|
| `coalesce_preserves_function` | Merging preserves layout eval | — |

### §3: Ceiling Division (Galois Connection)

| Theorem | Statement | Source |
|---------|-----------|--------|
| `ceilDiv_le_iff` | `⌈a/b⌉ ≤ Q ⟺ a ≤ Q×b` | Core insight |
| `ceilDiv_assoc` | `⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉` | integer-division.md §5.11 |
| `ceilDiv_of_dvd` | When divisible: `⌈n/d⌉ = n/d` | — |
| `ceilDiv_eq_div_add_one_of_not_dvd` | When indivisible: `⌈n/d⌉ = n/d + 1` | — |
| `ceilDiv_mul_ge_self` | `a ≤ ⌈a/b⌉ × b` | — |
| `ceilDiv_mul_sub_self_pos_of_not_dvd` | Indivisibility creates holes | — |
| `ceilDiv_eq_zero_iff` | `⌈a/b⌉ = 0 ⟺ a = 0` | Extension |
| `ceilDiv_eq_succ_iff` | Successor sandwich | Extension |
| `ceilDiv_mono_left` | Monotone in numerator | Extension |
| `ceilDiv_antitone_right` | Antitone in denominator | Extension |
| `ceilDiv_mul_sub_self_eq_zero_iff` | No holes ⟺ divisibility | Extension |

### §3: FTTC (Fundamental Theorem of TMA Correctness)

| Theorem | Statement | Source |
|---------|-----------|--------|
| `fttc` | `violated ⟺ ¬achievable` | tma-modeling-in-depth.md Thm 6 |
| `figure5_violated` | Example: e=3, B=5, S=8 violates FTTC | tma-modeling-in-depth.md Fig 5 |

**FTTC**: Strong correctness is **unachievable** iff:
```
e < B < S  ∧  e ∤ B
```
where `e` = element stride, `B` = box size, `S` = tensor size.

### §4: Integer Division

| Theorem | Statement | Source |
|---------|-----------|--------|
| `thm_2_5` | `r < a ⟹ r%a = r, r/a = 0` | integer-division.md §2.5 |
| `thm_2_7_1` | `a%c = 0 ⟹ (a+b)%c = b%c` | integer-division.md §2.7.1 |
| `thm_2_7_2` | `a%(b×c)%b = a%b` | integer-division.md §2.7.2 |
| `thm_2_9` | `b∣a ⟹ a%b = 0` | integer-division.md §2.9 |
| `thm_2_10` | `c∣b ⟹ a×(b/c) = (a×b)/c` | integer-division.md §2.10 |
| `thm_2_11` | `a/(b×c) = a/b/c` | integer-division.md §2.11 |
| `thm_2_12` | `a%(b×c) = a%b + (a/b%c)×b` | integer-division.md §2.12 |
| `thm_2_15_1` | `a/b%c = a%(b×c)/b` | integer-division.md §2.15.1 |
| `thm_2_16` | `i/d < D ⟺ i < D×d` | integer-division.md §2.16 |

### §4: IterDomain Operations

| Theorem | Statement | Source |
|---------|-----------|--------|
| `merge_split_identity` | `merge∘split = id ⟺ divisibility` | divisibility-of-split.md Thm 5 |
| `split_split_equivalence` | Two splits = one split by product | iterdomain.md Thm 2.1 |
| `predication_thm_1` | Outer bounds imply middle bounds | divisibility-of-split.md Thm 1 |
| `predication_thm_2` | Boundary equivalence | divisibility-of-split.md Thm 2 |

---

## Property Extraction

### How It Works

`PropertyExtract.lean` generates C++ property tests from the Lean theorems:

1. Each theorem becomes a `prop_*()` function in C++
2. Preconditions become early-return guards
3. The theorem statement becomes a boolean check
4. RapidCheck generates random inputs to test the properties

### Example

**Lean theorem:**
```lean
theorem ceilDiv_assoc (i m n : Nat) (hm : m > 0) (hn : n > 0) :
    ceilDiv (ceilDiv i n) m = ceilDiv i (m * n)
```

**Generated C++:**
```cpp
inline bool prop_ceilDiv_assoc(size_t i, size_t m, size_t n) {
    if (!(m > 0)) return true;  // precondition
    if (!(n > 0)) return true;  // precondition
    return ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n);
}
```

### Why Property Tests?

The tests are **redundant but essential**:
- **Redundant**: Lean already proved the properties for all valid inputs
- **Essential**: Validates that the C++ implementation matches the proven specification

---

## Documentation Reference

### doc/math/

| File | Lines | Content |
|------|-------|---------|
| `integer-division.md` | 942 | Euclidean, truncation, ceil division |
| `abstract-algebra.md` | 57 | Fields vs. Euclidean domains |
| `monotonic-function.md` | — | Monotonicity properties |
| `logic.md` | 35 | Boolean predicate simplification |

### doc/reading/

| File | Lines | Content |
|------|-------|---------|
| `tma-modeling-in-depth.md` | 545 | TMA correctness, FTTC, lowering |
| `divisibility-of-split.md` | 360 | Holes, predication, merge-split |
| `iterdomain.md` | 99 | IterDomain transformations |

### Key Concepts

**Holes**: When splitting an extent E by factor F where F∤E:
```
E=7, F=3 → outer=⌈7/3⌉=3, inner=3
product = 9 > 7 → 2 holes
```

**Weak vs Strong Correctness**:
- **Weak**: Valid items correct, holes can be garbage
- **Strong**: Valid items correct, holes filled with desired value (e.g., 0)

**TMA-Protected IterDomain**: An IterDomain whose out-of-bounds access
implies TMA's builtin predicate fails, ensuring automatic zero-filling.

---

## Statistics

```
Total theorems:     26
From nvfuser docs:  21
Extensions:          5
Sorry count:         0
```

---

## Design Philosophy

> "NVIDIA gave us the theorems. We gave them types."
> — VillaStraylight.lean coda

The project takes NVIDIA's informal proofs (markdown, SVG) and:
1. Formalizes them in Lean 4 with machine-checked proofs
2. Extracts property tests to validate C++ implementation
3. Ensures GPU tiling/scheduling correctness by construction

---

## References

- NVIDIA nvfuser documentation (BSD-3-Clause)
- Mathlib 4: https://github.com/leanprover-community/mathlib4
- Lean 4: https://lean-lang.org
- Gibson, William. *Neuromancer*. 1984.
