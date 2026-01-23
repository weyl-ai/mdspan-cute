# Lean Proofs and C++ Equivalence

This document explains how the Lean 4 formal proofs work, how they connect to the C++ implementation, and how to verify everything yourself.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     NVIDIA Documentation                         │
│   doc/math/integer-division.md, doc/reading/tma-modeling-*.md   │
│              ~30 files of handwritten proofs (BSD-3-Clause)      │
└────────────────────────────┬────────────────────────────────────┘
                             │ Formalized in
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   VillaStraylight.lean                           │
│              26 machine-verified theorems (0 sorry)              │
│                     Lean 4.15.0 + Mathlib                        │
└────────────────────────────┬────────────────────────────────────┘
                             │ Extracted via
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   PropertyExtract.lean                           │
│              Code generator: Lean theorems → C++ tests           │
└────────────────────────────┬────────────────────────────────────┘
                             │ Generates
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│              tests/property_tests.cpp (C++)                      │
│                    RapidCheck property tests                     │
│            Validates mdspan_cute matches the spec                │
└─────────────────────────────────────────────────────────────────┘
```

## What the Proofs Prove

The Lean proofs formalize NVIDIA's layout algebra - the mathematics behind GPU tensor tiling, swizzling, and memory access patterns. These aren't arbitrary theorems; they're the exact properties that NVIDIA engineers documented (informally) to ensure CUTLASS correctness.

### Key Theorem Categories

| Category | What It Proves | Why It Matters |
|----------|----------------|----------------|
| Coordinate Isomorphism | `decompose ∘ recompose = id` | Multi-index ↔ linear index is lossless |
| Coalescence | Merging modes preserves layout function | Mode fusion optimization is correct |
| Ceiling Division | Galois connection: `⌈a/b⌉ ≤ Q ⟺ a ≤ Q×b` | Extent calculations for splits are correct |
| FTTC | When TMA strong correctness is achievable | Prevents silent data corruption |
| Integer Division | `a / (b×c) = a/b/c`, `a % (b×c) = ...` | Split/merge transformations are correct |
| Predication | Bounds checks simplify correctly | Predicates can be hoisted/eliminated safely |

### The FTTC (Fundamental Theorem of TMA Correctness)

The most important theorem. Strong correctness is **unachievable** if and only if:

```
e < B < S  ∧  e ∤ B
```

Where:

- `e` = element stride
- `B` = box size
- `S` = tensor size

If this condition holds, your TMA will silently produce garbage. The Lean proof ensures the type system catches this at compile time.

## How Lean Proof Verification Works

### What Lean Does

Lean is a **proof assistant**: a programming language where types encode logical propositions and programs encode proofs. If a Lean file compiles, every theorem in it is mathematically proven.

```lean
-- This theorem states: for all natural numbers a, b with b > 0,
-- ceiling division is associative
theorem ceilDiv_assoc (i m n : ℕ) (hm : m > 0) (hn : n > 0) :
    ceilDiv (ceilDiv i n) m = ceilDiv i (m * n) := by
  -- The proof uses Mathlib lemmas about integer division
  simp only [ceilDiv]
  rw [Nat.add_sub_cancel]
  -- ... more tactics
  omega  -- Linear arithmetic solver finishes it
```

Key insight: `(hm : m > 0)` is not a runtime check - it's a **proof obligation**. You literally cannot call this theorem without providing mathematical evidence that `m > 0`.

### The Compilation = Verification Model

```bash
cd src/lean4-proof
lake build
```

If this succeeds with no errors:

- All 26 theorems are proven correct
- No `sorry` (proof holes) exist
- The type checker verified every logical step

If it fails, either:

- A proof is wrong (logic error)
- A proof is incomplete (`sorry` somewhere)
- Dependencies are broken (Mathlib version mismatch)

### Reading VillaStraylight.lean

The file uses a literary framing (Neuromancer quotes) but the mathematics is rigorous:

```lean
/-- Theorem 2.12 from integer-division.md: Mixed-radix decomposition.
    a % (b×c) = a%b + (a/b % c)×b -/
theorem thm_2_12 (a b c : ℕ) (hb : b > 0) (hc : c > 0) :
    a % (b * c) = a % b + (a / b % c) * b := by
  have h := Nat.div_add_mod a b
  omega
```

This directly corresponds to NVIDIA's `doc/math/integer-division.md` Theorem 2.12.

## How C++ Property Tests Connect

### The Redundancy Principle

The Lean proofs already prove these properties for **all** valid inputs. The C++ tests are **redundant** - but essential for a different reason:

> Lean proves the **specification** is correct.
> C++ tests verify the **implementation** matches the specification.

### PropertyExtract.lean

This file defines a code generator that extracts each Lean theorem into a C++ property test:

```lean
def properties : List PropertySpec := [
  { name := "ceilDiv_assoc"
    theorem_name := "ceilDiv_assoc"
    params := [("i", "size_t"), ("m", "size_t"), ("n", "size_t")]
    preconditions := ["m > 0", "n > 0"]
    property := "ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n)"
    doc := "Ceiling division is associative"
  },
  -- ... 25 more theorems
]
```

This generates `property_tests.hpp`:

```cpp
inline bool prop_ceilDiv_assoc(size_t i, size_t m, size_t n) {
    if (!(m > 0)) return true;  // precondition
    if (!(n > 0)) return true;  // precondition
    return ceilDiv(ceilDiv(i, n), m) == ceilDiv(i, m * n);
}
```

### RapidCheck Integration

The property tests use [RapidCheck](https://github.com/emil-e/rapidcheck) for property-based testing:

```cpp
TEST_CASE("Villa Straylight Theorems", "[properties][lean]") {
    SECTION("ceilDiv_assoc") {
        rc::check([](void) {
            auto i = *rc::gen::inRange<size_t>(0, 10000);
            auto m = *rc::gen::inRange<size_t>(1, 1000);  // > 0
            auto n = *rc::gen::inRange<size_t>(1, 1000);  // > 0
            RC_ASSERT(prop_ceilDiv_assoc(i, m, n));
        });
    }
}
```

RapidCheck generates random inputs satisfying the preconditions and checks that the property holds. If it finds a counterexample, your C++ implementation doesn't match the proven specification.

## Running the Verification

### Prerequisites

```bash
# Lean toolchain (for proofs)
curl https://raw.githubusercontent.com/leanprover/elan/master/elan-init.sh -sSf | sh

# Or use Nix
nix develop .#lean
```

### Verify Lean Proofs

```bash
cd src/lean4-proof

# Download pre-built Mathlib (REQUIRED - don't build from source)
lake exe cache get

# Build and verify all theorems
lake build

# Success = all 26 theorems verified
```

### Run C++ Property Tests

```bash
# With Buck2 (recommended)
nix develop --command buck2 test //src/mdspan-cute:property_tests
```

Expected output:

```
===============================================================================
All tests passed (98993 assertions in 59 test cases)
```

### Generate Fresh Property Tests (Optional)

If you modify VillaStraylight.lean and add new theorems:

```bash
cd src/lean4-proof
lake exe extract-properties
# Writes to src/lean4-proof/extracted/cpp/
```

Then update `src/mdspan-cute/tests/property_tests.cpp` to include the new properties.

## Theorem Reference

### §1: Coordinate Isomorphism

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `recompose_decompose_2d` | Round-trip is identity | `recompose2(M0, decompose2(M0, M1, x)) == x` |
| `decompose_recompose_2d` | Inverse direction | `decompose2(M0, M1, recompose2(M0, x0, x1)) == {x0, x1}` |

### §2: Coalescence

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `coalesce_preserves_function` | Mode merging is correct | `x0*d0 + x1*d1 == (x0 + x1*s0)*d0` when `d1 == s0*d0` |

### §3: Ceiling Division (Galois Connection)

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `ceilDiv_le_iff` | **THE** Galois connection | `(ceilDiv(a,b) <= Q) == (a <= Q*b)` |
| `ceilDiv_assoc` | Associativity | `ceilDiv(ceilDiv(i,n),m) == ceilDiv(i, m*n)` |
| `ceilDiv_of_dvd` | Divisible case | `n % d == 0 ⟹ ceilDiv(n,d) == n/d` |
| `ceilDiv_eq_div_add_one_of_not_dvd` | Indivisible case | `n % d != 0 ⟹ ceilDiv(n,d) == n/d + 1` |
| `ceilDiv_mul_sub_self_pos_of_not_dvd` | Holes exist | `n % d != 0 ⟹ ceilDiv(n,d)*d - n > 0` |

### §3: FTTC

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `fttc` | Strong correctness criterion | `(e<B ∧ B<S ∧ B%e≠0) == ¬(B%e=0 ∨ B≥S ∨ e≥B)` |

### §4: Integer Division

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `thm_2_5` | Small numbers | `r < a ⟹ r%a == r ∧ r/a == 0` |
| `thm_2_7_1` | Adding multiples | `a%c == 0 ⟹ (a+b)%c == b%c` |
| `thm_2_7_2` | Nested mod | `a % (b*c) % b == a % b` |
| `thm_2_10` | Div distributes | `b%c == 0 ⟹ a*(b/c) == (a*b)/c` |
| `thm_2_11` | Div associates | `a / (b*c) == a/b/c` |
| `thm_2_12` | **Mixed-radix** | `a % (b*c) == a%b + (a/b % c)*b` |
| `thm_2_15_1` | Middle digit | `a/b % c == a % (b*c) / b` |
| `thm_2_16` | **Bound theorem** | `(i/d < D) == (i < D*d)` |

### §4: IterDomain Operations

| Theorem | Statement | C++ Property |
|---------|-----------|--------------|
| `merge_split_identity` | merge∘split = id criterion | `ceilDiv(e,f)*f == e ⟺ e%f == 0` |
| `split_split_equivalence` | Two splits = one split | `ceilDiv(ceilDiv(i,n),m) == ceilDiv(i, m*n)` |

## Troubleshooting

### "lake exe cache get" fails

Your Lean version doesn't match Mathlib. Check `lean-toolchain`:

```bash
cat src/lean4-proof/lean-toolchain
# Should show: leanprover/lean4:v4.15.0
```

### "object file does not exist"

You didn't run `lake exe cache get` first. Mathlib takes hours to build from source; always use the cache.

### Property test fails

This means the C++ implementation doesn't match the Lean specification. Either:

1. Bug in C++ code
1. Bug in property extraction
1. Integer overflow (tests use `size_t`)

Check the failing input - RapidCheck prints the minimal counterexample.

### Out of memory

Mathlib is large (~8GB RAM to build). Use `lake exe cache get` to download pre-built artifacts.

## Further Reading

- `doc/math/integer-division.md` - 942 lines of division properties
- `doc/reading/tma-modeling-in-depth.md` - TMA correctness model, FTTC
- `doc/reading/divisibility-of-split.md` - Holes, predication, merge-split
- [Mathlib documentation](https://leanprover-community.github.io/mathlib4_docs/)
- [Lean 4 manual](https://lean-lang.org/lean4/doc/)
