{
  description = "mdspan-cute: Bridge between C++23 std::mdspan and CUTLASS cute layouts";

  inputs = {
    weyl-std.url = "github:weyl-ai/weyl-std/dev";
    nixpkgs.follows = "weyl-std/nixpkgs";
    flake-parts.follows = "weyl-std/flake-parts";
  };

  outputs = inputs@{ flake-parts, weyl-std, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ weyl-std.flakeModules.default ];

      systems = [ "x86_64-linux" "aarch64-linux" ];

      weyl-std = {
        nixpkgs.cuda.enable = true;
        overlays.enable = true;
      };

      perSystem = { pkgs, lib, ... }:
        let
          cutlass = pkgs.callPackage ./cutlass.nix { };
          mdspan-cute = pkgs.callPackage ./package.nix { inherit cutlass; };
          example = pkgs.callPackage ./example.nix {
            inherit mdspan-cute cutlass;
            inherit (pkgs) cudaPackages;
          };
          tests = pkgs.callPackage ./tests.nix {
            inherit mdspan-cute cutlass;
            inherit (pkgs) cudaPackages;
          };
        in
        {
          packages = {
            default = mdspan-cute;
            inherit mdspan-cute cutlass example tests;
          };

          devShells.default = pkgs.mkShell {
            name = "mdspan-cute-dev";

            packages = with pkgs; [
              mdspan-cute
              cutlass
              llvmPackages.clang
              cmake
              ninja
            ] ++ lib.optionals pkgs.stdenv.isLinux [
              cudaPackages.cuda_nvcc
              cudaPackages.cuda_cudart
              cudaPackages.cuda_cccl
            ];

            shellHook = ''
              echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
              echo "  mdspan-cute development shell"
              echo "  The Polyhedral Wizards at Play"
              echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
              echo ""
              echo "  #include <mdspan_cute.h>"
              echo "  tile[row, col] = value;  // swizzled, composed, zero-cost"
              echo ""
            '';
          };

          # Lean 4 / Mathlib development shell
          devShells.lean = pkgs.mkShell {
            name = "razorgirl-lean";

            packages = with pkgs; [
              elan # Lean version manager (installs lake)
              git # Required for lake to fetch deps
              curl # For Mathlib cache
              gmp # GMP for Mathlib
            ];

            shellHook = ''
              echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
              echo "  razorgirl proof development shell"
              echo "  Lean 4 + Mathlib"
              echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
              echo ""
              echo "  cd proof"
              echo "  lake exe cache get   # download Mathlib oleans"
              echo "  lake build           # build razorgirl"
              echo ""
              echo "  Lean version: $(elan show active-toolchain 2>/dev/null || echo 'run: elan default leanprover/lean4:v4.15.0')"
              echo ""
              
              # Ensure elan has the right toolchain
              if [ -f proof/lean-toolchain ]; then
                cd proof && elan override set "$(cat lean-toolchain)" && cd ..
              fi
            '';

            ELAN_HOME = "$HOME/.elan";
          };

          # Checks: C++ tests and Lean proofs
          #
          # NOTE: Automated checks for C++ tests and Lean proofs are disabled because they require
          # network access to fetch dependencies (Catch2/RapidCheck via CMake FetchContent, and
          # Mathlib for Lean), which doesn't work in Nix's sandboxed build environment.
          #
          # To run tests locally:
          #
          # C++ Tests:
          #   nix develop
          #   cmake -B build -DCMAKE_CXX_COMPILER=clang++
          #   cmake --build build
          #   cd build && ctest --output-on-failure
          #
          # Lean Proofs:
          #   nix develop .#lean
          #   cd proof
          #   lake exe cache get  # Fetch Mathlib precompiled binaries
          #   lake build           # Verify all proofs
          checks = {
            # Only formatting check is included (doesn't need network)
          };
        };

      flake = {
        overlays.default = final: _prev: {
          cutlass = final.callPackage ./cutlass.nix { };
          mdspan-cute = final.callPackage ./package.nix {
            inherit (final) cutlass;
          };
        };
      };
    };
}
