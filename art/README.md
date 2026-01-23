# The Villa Straylight Papers: Visual Gallery

*"Two decades of work, one line of code"*

Educational SVG visualizations for **A Formal Treatment of NVIDIA's Layout Algebra**.
Each piece uses the `ono-sendai-razorgirl` palette and Berkeley Mono typography.
Rendered at 1920×1080 for desktop wallpapers and documentation.

## The Collection

### 00-dedication.svg

**The Polyhedral Wizards at Play**

A dedication to those who gave us the algebra of tensors:

- **Polyhedral Foundations**: Feautrier, Bastoul, Cohen, Verdoolaege (isl)
- **C++23 mdspan**: Bryce Lelbach, Christian Trott, H. Carter Edwards, The Kokkos Team
- **CUTLASS CuTe**: Andrew Kerr, Duane Merrill, Haicheng Wu, NVIDIA CUTLASS Team

*"mdspan-cute: where two decades of work become one line of code"*

______________________________________________________________________

### 00-title.svg

**The Villa Straylight Papers**

Title card featuring:

- 21 theorems
- 0 `sorry`
- ∞ TFLOPS

*"The Villa Straylight knows no sky, recorded or otherwise."*

______________________________________________________________________

### 01-coordinate-isomorphism.svg

**The Bijection**

Shows how 1D indices map to nD coordinates through decompose/recompose.

______________________________________________________________________

### 06-galois-connection.svg

**Ceiling Division as a Galois Connection**

The fundamental adjunction: `ceilDiv(a, b) ≤ c ⟺ a ≤ c * b`

______________________________________________________________________

### 08-split-split.svg

**Split-Split Equivalence**

Theorem 2.1 from `iterdomain.md`:

```
Split(Split(I{M}, N₀), N₁) ≈ Split(I{M}, N₀ × N₁)
```

______________________________________________________________________

### 09-merge-split.svg

**Merge-Split Operations**

The inverse relationship between merge and split transformations.

______________________________________________________________________

### 10-ceiling-assoc.svg

**Ceiling Division Associativity**

Shows when `ceilDiv(ceilDiv(a, b), c) = ceilDiv(a, b * c)`.

______________________________________________________________________

### 11-division-pitfalls.svg

**The Pitfalls of Division in ℕ**

Common mistakes when working with natural number division.

______________________________________________________________________

### 12-coord-isomorphism.svg

**Coordinate Bijection (Extended)**

Detailed visualization of the decompose/recompose isomorphism.

______________________________________________________________________

### 13-ceiling-vs-floor.svg

**Ceiling vs Floor Division**

The critical distinction: `ceilDiv(a, b) = ⌊(a + b - 1) / b⌋`

______________________________________________________________________

### 14-division-assoc.svg

**Division Associativity**

When does `(a / b) / c = a / (b * c)`?

______________________________________________________________________

### 15-fttc.svg

**The Fundamental Theorem of TMA Correctness**

Source: `tma-modeling-in-depth.md` · Theorem 6

Strong correctness is UNACHIEVABLE iff: `e < B < S ∧ e ∤ B`

Shows:

- The violation case: element splits across TMA boxes
- The three escape hatches: `e|B`, `B≥S`, or `e≥B`
- Type system implications: proof obligations prevent invalid schedules

*"The shotgun wired to the forehead"*

______________________________________________________________________

### 16-coalescence.svg

**Coalescence: Direction Matters**

Source: Lei Mao's CuTe Layout Blog

The wrong direction bug: merging modes in the wrong order scrambles data.

Shows when `d₁ = N₀ × d₀` enables safe coalescing.

______________________________________________________________________

### 17-holes.svg

**Indivisible Split Creates Holes**

Source: `divisibility-of-split.md`

The core bug pattern: `Split(I{6}, 4)` creates 2 holes because `ceilDiv(6,4) × 4 = 8 ≠ 6`.

Shows:

- Original 1D array: 6 valid items
- After split: 2×4 grid with 2 holes (indices 6, 7)
- The fix: add predicates `if i0*4+i1 < 6`

______________________________________________________________________

### 18-bound-theorem.svg

**Theorem 4.9: The Bound Theorem**

```
i / d < D  ⟺  i < D × d
```

The fundamental theorem for split validation. Essential for predication.

______________________________________________________________________

### 19-predication.svg

**Predication Theorems**

Theorems 4.12-4.13: How to validate split bounds efficiently.

Shows when you can skip checking middle indices.

______________________________________________________________________

### 20-mode-layout.svg

**Mode Layouts in CuTe**

How shape, stride, and swizzle compose to form layouts.

______________________________________________________________________

### 21-jensens-razor.svg

**Jensen's Razor**

*"What can be composed, shall be composed."*

The design philosophy behind CuTe's layout algebra.

______________________________________________________________________

### 22-division-algorithm.svg

**The Division Algorithm**

For all `a, b ∈ ℕ` with `b > 0`:

```
∃!q, r : a = b·q + r  ∧  r < b
```

The foundation of mixed-radix decomposition.

______________________________________________________________________

### 25-proof-index.svg

**Proof Index**

Complete listing of all 21 theorems with their locations in the codebase.

______________________________________________________________________

### 26-villa-straylight.svg

**The Villa Straylight (Wallpaper)**

Desktop wallpaper featuring the ono-sendai-razorgirl aesthetic.
The heart of Tessier-Ashpool in cyberspace.

______________________________________________________________________

### 29-galois-connection.svg

**Galois Connections in Action**

Extended treatment of the ceiling division adjunction.

______________________________________________________________________

### 30-coda.svg

**Coda**

*"All the speed of light and dreams..."*

The conclusion of The Villa Straylight Papers.

______________________________________________________________________

## Design Language

**Palette**: ono-sendai-razorgirl

- Base: `#111417` (base00)
- Highlights: `#54aeff`, `#80ccff`, `#b6e3ff` (cyan spectrum)
- Muted: `#596775` (base03)

**Typography**: Berkeley Mono (with fallbacks to SF Mono, Consolas)

**Grid**: Subtle 64px polyhedral grid pattern at 15% opacity

**Animations**: Blinking cursor at key code snippets

## Usage

These SVGs are designed for:

- Desktop wallpapers (1920×1080)
- Documentation illustrations
- Teaching materials
- Conference slides
- Paper figures

All are resolution-independent vectors that scale perfectly.

## License

Art: CC-BY-4.0
Theorems: From NVIDIA nvfuser (BSD-3-Clause), The Villa Straylight Papers (Apache-2.0)

## Acknowledgments

Dedicated to the polyhedral wizards, the mdspan standardizers, and the CUTLASS architects
who built the foundations we stand on.

*"mdspan-cute: the zero-overhead bridge"*
