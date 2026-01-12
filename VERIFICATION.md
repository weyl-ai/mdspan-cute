# Verification Status

**The Villa Straylight Papers: A Formal Treatment of NVIDIA's Layout Algebra**

This document tracks what's **proven** vs **tested** vs **documented**.

All theorems cite nvfuser documentation (BSD-3-Clause, NVIDIA) and original work.

## Source Documents

Archived in `proof/doc/`:

```
doc/math/
├── integer-division.md      # 928 lines - Euclidean/truncation/ceiling division
├── monotonic-function.md    # Monotonicity lemmas
├── logic.md                 # Boolean simplification
└── abstract-algebra.md      # Why integers aren't a field

doc/reading/
├── divisibility-of-split.md # Holes, predication, correctness models
├── tma-modeling-in-depth.md # TMA, FTTC, three levels
└── iterdomain.md            # Transformation algebra
```

## Lean 4 Proofs

**Status: 21 theorems, 0 `sorry`, fully proven ✓**

### VillaStraylight.lean - Complete Formalization

All proofs verified with Lean 4.15.0 + Mathlib 4.15.0.

#### §1. Ceiling Division (6 theorems)

| Theorem | Lines | What it proves |
|---------|-------|----------------|
| `ceilDiv_pos` | 492-496 | Ceiling div is positive |
| `ceilDiv_mul_ge` | 523-537 | a ≤ ⌈a/b⌉ × b (Galois lower bound) |
| `ceilDiv_le` | 544-560 | ⌈a/b⌉ ≤ c ⟺ a ≤ c × b (Galois connection) |
| `ceilDiv_of_dvd` | 640-660 | ⌈a/b⌉ = a/b when b \| a |
| `ceilDiv_eq_div_add_one_of_not_dvd` | 671-695 | ⌈a/b⌉ = a/b + 1 when b ∤ a |
| `ceilDiv_monotone_num` | 719-724 | Monotonic in numerator |

#### §2. Division Algebra (4 theorems)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `thm_2_5` | integer-division.md 2.5 | r % a = r when r < a |
| `thm_2_7_1` | integer-division.md 2.7 | (a + b) % c = b % c when a % c = 0 |
| `thm_2_10` | integer-division.md 2.10 | a × (b/c) = (a×b)/c when c \| b |
| `thm_2_11` | integer-division.md 2.11 | a/(b×c) = (a/b)/c |

#### §3. Mixed-Radix Decomposition (3 theorems)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `recompose_decompose_2d` | - | Coordinate round-trip |
| `thm_2_12` | integer-division.md 2.12 | a % (b×c) = a%b + (a/b%c)×b |
| `thm_2_15_1` | integer-division.md 2.15 | a/b%c = a%(b×c)/b (extract middle digit) |

#### §4. The Bound Theorem (3 theorems)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `thm_2_16` | integer-division.md 2.16 | i/d < D ⟺ i < D×d (THE bound theorem) |
| `predication_thm_1` | divisibility-of-split.md Thm 4.12 | Split: if i0<N0 ∧ i2<N2 then i1<⌈N0/N2⌉ |
| `predication_thm_2` | divisibility-of-split.md Thm 4.13 | Merge: i0<N0 ⟺ i2<N2 |

#### §5. Split-Merge Algebra (3 theorems)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `split_split_equivalence` | iterdomain.md Thm 2.1 | Split(Split(M, N₀), N₁) ≈ Split(M, N₀×N₁) |
| `merge_split_merge` | divisibility-of-split.md | Merge-split composition |
| `split_merge_split` | divisibility-of-split.md | Split-merge composition |

#### §6. Swizzle Algebra (1 theorem)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `swizzle_involutive` | - | Swizzle is self-inverse |

#### §7. The Fundamental Theorem of TMA Correctness (1 theorem)

| Theorem | Source | What it proves |
|---------|--------|----------------|
| `fttc` | tma-modeling-in-depth.md Thm 6 | Strong correctness UNACHIEVABLE iff: e < B < S ∧ e ∤ B |

**The shotgun wired to the forehead**: Type system prevents invalid TMA schedules.

## Property Tests

### tests/property_tests.cpp - RapidCheck

#### Integer Division Theorems

| Test | Citation | Coverage |
|------|----------|----------|
| `nvfuser Theorem 2.10` | integer-division.md | Random (a,b,c) where c\|b |
| `nvfuser Theorem 2.11` | integer-division.md | Random (a,b,c) |
| `nvfuser Theorem 2.12` | integer-division.md | Random (a,b,c) |
| `nvfuser Theorem 2.16` | integer-division.md | Random (i,D,d) |

#### FTTC

| Test | Citation | Coverage |
|------|----------|----------|
| `FTTC exhaustive` | tma-modeling-in-depth.md | All e≤16, B≤32, S≤64 |
| `FTTC worked examples` | tma-modeling-in-depth.md | Specific configs from doc |

#### Predication

| Test | Citation | Coverage |
|------|----------|----------|
| `Predication Theorem 1` | divisibility-of-split.md | Split bound implication |
| `Predication Theorem 2` | divisibility-of-split.md | Merge bound equivalence |
| `Predication Theorem 3` | divisibility-of-split.md | Resize bound implication |

#### Correctness Model

| Test | Citation | Coverage |
|------|----------|----------|
| `indivisible split creates holes` | divisibility-of-split.md | Split(I{6}, 4) creates 2 holes |
| `divisible split no holes` | divisibility-of-split.md | Split(I{6}, 2) creates 0 holes |

### tests/test_layout_cute.cpp - mdspan Bridge Tests

| Test | What it verifies |
|------|------------------|
| `layout_cute::operator()` | Coordinate → offset mapping matches CuTe |
| `mdspan bracket syntax` | C++23 `tensor[i,j,k]` works with swizzled layouts |
| `composition correctness` | Swizzle ∘ Base layout composition |
| `zero overhead` | No runtime cost vs direct CuTe |

## Integration Tests

### examples/swizzled_tile.cpp

**The Polyhedral Wizards at Play**: Demonstrates that two decades of work becomes one line of code:

```cpp
tile[row, col] = value;  // swizzled, composed, zero-cost
```

Verifies:
- C++23 bracket syntax with CuTe swizzled layouts
- Bank-conflict-free shared memory access with `Swizzle<3,3,3>`
- Zero-overhead abstraction (compiles to identical assembly)

## Verification Matrix

| Invariant | Lean Proof | Property Test | Integration Test | Source |
|-----------|:----------:|:-------------:|:----------------:|--------|
| Coordinate isomorphism | ✓ | - | - | Original |
| Ceiling div Galois connection | ✓ | - | - | Original |
| FTTC condition | ✓ | ✓ (exhaustive) | - | Thm 6 |
| Split predication | ✓ | ✓ | - | Thm 4.12 |
| Merge predication | ✓ | ✓ | - | Thm 4.13 |
| Divisible split no holes | ✓ | ✓ | - | Original |
| Split-split equivalence | ✓ | ✓ | - | iterdomain Thm 2.1 |
| Merge-split composition | ✓ | - | - | Original |
| Swizzle involutive | ✓ | - | ✓ | Original |
| Thm 2.10 (divisible mul) | ✓ | ✓ | - | integer-division.md |
| Thm 2.11 (div assoc) | ✓ | ✓ | - | integer-division.md |
| Thm 2.12 (mod decomp) | ✓ | ✓ | - | integer-division.md |
| Thm 2.16 (bound) | ✓ | ✓ | - | integer-division.md |
| mdspan-cute bridge | - | ✓ | ✓ | Original |

## How to Verify

### Lean 4 Proofs

```bash
nix develop .#lean
cd proof
lake exe cache get  # Download Mathlib precompiled binaries
lake build           # Verify all 21 theorems
```

**Expected output:**
```
✔ [2522/2523] Built VillaStraylight
Build completed successfully.
```

### Property Tests

```bash
nix develop
cmake -B build -DCMAKE_CXX_COMPILER=clang++
cmake --build build
./build/property_tests
```

**Run specific test categories:**

```bash
./build/property_tests "[fttc]"           # FTTC tests
./build/property_tests "[nvfuser]"        # nvfuser theorem tests
./build/property_tests "[correctness]"    # Correctness model tests
```

### Integration Test

```bash
./build/swizzled_tile
```

**Expected output:**
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  The Polyhedral Wizards at Play
  mdspan + cute: two decades of work, one line of code
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Sample values from swizzled tile:
  tile[0, 0]   = 0
  tile[16, 16] = 1616
  ...

Swizzle pattern (logical → physical offset):
  row 0: [0→0:b0] [1→1:b1] ...
  row 2: [64→72:b8] [65→73:b9] ...  // XOR swizzle prevents bank conflicts
```

## The Value Proposition

| Bug Pattern | Runtime Behavior | With Villa Straylight | Source |
|-------------|------------------|----------------------|--------|
| Indivisible split (128×48) | SIGSEGV or corruption | Type error at compile time | divisibility-of-split.md |
| FTTC violation (e∤B, e<B<S) | Garbage in tensor cores | Type error at compile time | tma-modeling-in-depth.md |
| Wrong coalescence direction | Scrambled data | Type error at compile time | Lei Mao |
| Merge-split without divisibility | Wrong iteration order | Type error at compile time | Theorem 2.1 |
| Missing predicate after split | Out-of-bounds access | Type error at compile time | Theorem 4.12 |

**Time saved**: Days of CUDA debugging → seconds of compile errors.

## Visual Proofs

23 SVG visualizations in `art/` directory using the ono-sendai-razorgirl palette:

- `00-dedication.svg` - The Polyhedral Wizards at Play
- `15-fttc.svg` - The Fundamental Theorem of TMA Correctness
- `17-holes.svg` - Indivisible split creates holes
- `18-bound-theorem.svg` - Theorem 4.9: i/d < D ⟺ i < D×d
- `26-villa-straylight.svg` - Desktop wallpaper (1920×1080)
- ...and 18 more

Each visualization is a 1920×1080 resolution-independent SVG suitable for:
- Desktop wallpapers
- Documentation figures
- Conference slides
- Teaching materials

## Citation

```bibtex
@software{villa_straylight_papers,
  title = {The Villa Straylight Papers: A Formal Treatment of NVIDIA's Layout Algebra},
  author = {razorgirl / Weyl AI},
  year = {2026},
  note = {Theorems from NVIDIA nvfuser (BSD-3-Clause),
          mdspan-cute bridge (Apache-2.0),
          Dedicated to the polyhedral wizards who built the foundations}
}
```

## Acknowledgments

This work bridges decades of research:

**Polyhedral Foundations**: Paul Feautrier, Cédric Bastoul, Albert Cohen, Sven Verdoolaege

**C++23 mdspan**: Bryce Adelstein Lelbach, Christian Trott, H. Carter Edwards, The Kokkos Team

**CUTLASS CuTe**: Andrew Kerr, Duane Merrill, Haicheng Wu, NVIDIA CUTLASS Team

*"Two decades of work, one line of code"*
