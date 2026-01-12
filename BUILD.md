# Building mdspan-cute

## Quick Start

### Using Nix (Recommended)

Build the library:
```bash
nix build
```

Enter development shell:
```bash
nix develop
```

### Building Examples and Tests

Due to CMake's FetchContent requiring network access to download dependencies (mdspan, Catch2, RapidCheck), the example and test derivations cannot be built in Nix's sandboxed environment.

**Build locally using the dev shell:**

```bash
# Enter the development shell
nix develop

# Configure and build
cmake -B build -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j$(nproc)

# Run the example
./build/swizzled_tile

# Run tests
cd build && ctest --output-on-failure
```

**Output:**
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  The Polyhedral Wizards at Play
  mdspan + cute: two decades of work, one line of code
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Sample values from swizzled tile:
  tile[0, 0]   = 0
  tile[0, 1]   = 1
  tile[16, 16] = 1616
  ...
```

## Flake Outputs

- `nix build` / `nix build .#mdspan-cute` - Build the library (headers only)
- `nix build .#cutlass` - Build CUTLASS dependency
- `nix develop` - Enter C++ development shell with all dependencies
- `nix develop .#lean` - Enter Lean 4 proof development shell
- `nix flake show` - Show all available outputs

## Lean 4 Proofs

Build The Villa Straylight Papers (formal proofs):

```bash
nix develop .#lean
cd proof
lake exe cache get  # Download Mathlib precompiled binaries
lake build           # Verify all 21 theorems
```

All proofs compile with zero `sorry`. ✓

## Dependencies

### C++ Library (Header-only)
- C++23 compiler (Clang 15+ or GCC 13+)
- CUTLASS 4.3+ (provided by flake)
- C++23 `<mdspan>` (fetched by CMake)

### Examples and Tests
- CUDA 12.8+ headers (provided by flake)
- Catch2 v3 (fetched by CMake)
- RapidCheck (fetched by CMake)

### Lean 4 Proofs
- Lean 4.15.0 (managed by elan)
- Mathlib 4.15.0 (fetched by lake)

## Project Structure

```
mdspan-cute/
├── include/mdspan_cute.h          # Main header
├── include/mdspan_cute/
│   ├── layout_cute.h               # C++23 mdspan layout adapter
│   └── cuda_gcc15_compat.h         # Compatibility shims
├── examples/
│   └── swizzled_tile.cpp           # Demo: C++23 syntax + cute swizzle
├── tests/
│   ├── test_layout_cute.cpp        # Layout bridge tests
│   └── property_tests.cpp          # Property-based tests
├── proof/
│   └── VillaStraylight.lean        # Formal proofs (21 theorems)
├── art/                            # Visual proofs (SVG wallpapers)
│   ├── 00-dedication.svg           # The Polyhedral Wizards
│   ├── 15-fttc.svg                 # Fundamental Theorem of TMA Correctness
│   └── ...                         # 23 total visualizations
├── flake.nix                       # Nix flake configuration
├── example.nix                     # Example derivation (local build only)
├── tests.nix                       # Tests derivation (local build only)
└── CMakeLists.txt                  # CMake configuration
```

## CI/CD Notes

The `example` and `tests` packages in `flake.nix` are provided for local development but cannot be built in sandboxed CI environments without pre-fetching dependencies.

For CI, consider:
1. Using the `nix develop` shell and building with CMake directly
2. Pre-fetching dependencies as separate fixed-output derivations
3. Using system-provided Catch2/RapidCheck instead of FetchContent

## License

- Code: Apache-2.0
- Art: CC-BY-4.0
- Theorems: From NVIDIA nvfuser (BSD-3-Clause) and original work (Apache-2.0)
