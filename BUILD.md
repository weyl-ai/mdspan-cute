# Building mdspan-cute

## Quick Start

### Using Nix + Buck2 (Recommended)

Enter development shell:

```bash
nix develop
```

Build all targets:

```bash
buck2 build //...
```

Run all tests:

```bash
buck2 test //...
```

**Expected output:**

```
✓ Pass: root//src/mdspan-cute:layout_cute_tests
✓ Pass: root//src/mdspan-cute:property_tests
Tests finished: Pass 2. Fail 0.
```

## Flake Outputs

- `nix develop` - Enter C++ development shell with all dependencies
- `nix develop .#lean` - Enter Lean 4 proof development shell
- `nix build` / `nix build .#mdspan-cute` - Build the library (headers only)
- `nix build .#cutlass` - Build CUTLASS dependency
- `nix build .#tests` - Build and run tests via Buck2
- `nix flake show` - Show all available outputs

## Lean 4 Proofs

Build The Villa Straylight Papers (formal proofs):

```bash
nix develop .#lean
cd src/lean4-proof
lake exe cache get  # Download Mathlib precompiled binaries
lake build          # Verify all 26 theorems
```

All proofs compile with zero `sorry`. ✓

## Dependencies

### C++ Library (Header-only)

- C++23 compiler (Clang 15+ or GCC 13+)
- CUTLASS 4.3+ (provided by flake)
- mdspan reference implementation (provided by flake)

### Tests

- Catch2 v3 (provided by flake)
- RapidCheck (provided by flake)

### Lean 4 Proofs

- Lean 4.15.0 (managed by elan)
- Mathlib 4.15.0 (fetched by lake)

## Project Structure

```
mdspan-cute/
├── src/
│   ├── mdspan-cute/
│   │   ├── include/mdspan_cute.h       # Main header
│   │   ├── include/mdspan_cute/
│   │   │   ├── layout_cute.h           # C++23 mdspan layout adapter
│   │   │   └── cuda_gcc15_compat.h     # Compatibility shims
│   │   ├── examples/
│   │   │   └── swizzled_tile.cpp       # Demo: C++23 syntax + cute swizzle
│   │   ├── tests/
│   │   │   ├── test_layout_cute.cpp    # Layout bridge tests
│   │   │   └── property_tests.cpp      # Property-based tests
│   │   └── BUCK                        # Buck2 build targets
│   └── lean4-proof/
│       ├── VillaStraylight.lean        # Formal proofs (26 theorems)
│       ├── PropertyExtract.lean        # C++ test code generator
│       ├── extracted/cpp/              # Generated property tests
│       └── doc/                        # Mathematical documentation
├── art/                                # Visual proofs (SVG wallpapers)
│   ├── 00-dedication.svg               # The Polyhedral Wizards
│   ├── 15-fttc.svg                     # Fundamental Theorem of TMA Correctness
│   └── ...                             # 23 total visualizations
├── bin/                                # Compiler wrapper scripts
├── toolchains/                         # Buck2 toolchain definitions
├── third_party/                        # External dependency declarations
├── flake.nix                           # Nix flake configuration
├── .buckconfig                         # Buck2 configuration
└── BUCK                                # Root build target
```

## Buck2 Targets

```bash
# Build library
buck2 build //src/mdspan-cute:mdspan-cute

# Run specific test
buck2 test //src/mdspan-cute:layout_cute_tests
buck2 test //src/mdspan-cute:property_tests

# Build all
buck2 build //...
```

## CI/CD Notes

The build uses Buck2 with Nix-provided dependencies. All dependencies come from the Nix environment via `NIX_CFLAGS_COMPILE`, so no network access is required during the Buck2 build.

For CI, use:

```bash
nix develop --command bash -c 'buck2 build //... && buck2 test //...'
```

## License

- Code: Apache-2.0
- Art: CC-BY-4.0
- Theorems: From NVIDIA nvfuser (BSD-3-Clause) and original work (Apache-2.0)
