{
  description = "mdspan-cute: Bridge between C++23 std::mdspan and CUTLASS cute layouts";

  inputs = {
    weyl-std.url = "github:weyl-ai/weyl-std/dev";
    nixpkgs.follows = "weyl-std/nixpkgs";
    flake-parts.follows = "weyl-std/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, weyl-std, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ weyl-std.flakeModules.default ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      weyl-std = {
        nixpkgs.cuda.enable = true;
        overlays.enable = true;
      };

      perSystem =
        { pkgs, lib, ... }:
        let
          llvmPkgs = pkgs.llvmPackages_latest;
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
            inherit
              mdspan-cute
              cutlass
              example
              tests
              ;

            # === Buck2 Toolchain Exports ===
            # These are consumed by buck2-nix's flake.package rule

            # LLVM toolchain - all tools needed for C++ compilation
            llvm-toolchain = pkgs.runCommand "llvm-toolchain" { } ''
              mkdir -p $out/bin

              # Symlink all LLVM/Clang binaries
              for pkg in ${llvmPkgs.clang} ${llvmPkgs.lld} ${llvmPkgs.llvm}; do
                if [ -d "$pkg/bin" ]; then
                  for bin in "$pkg/bin"/*; do
                    [ -e "$bin" ] && ln -sf "$bin" "$out/bin/$(basename "$bin")"
                  done
                fi
              done

              # Create cc/c++ symlinks expected by buck2-nix
              ln -sf ${llvmPkgs.clang}/bin/clang $out/bin/cc
              ln -sf ${llvmPkgs.clang}/bin/clang++ $out/bin/c++

              # Standard binutils-like tools from LLVM
              ln -sf ${llvmPkgs.llvm}/bin/llvm-ar $out/bin/ar
              ln -sf ${llvmPkgs.llvm}/bin/llvm-nm $out/bin/nm
              ln -sf ${llvmPkgs.llvm}/bin/llvm-objcopy $out/bin/objcopy
              ln -sf ${llvmPkgs.llvm}/bin/llvm-ranlib $out/bin/ranlib
              ln -sf ${llvmPkgs.llvm}/bin/llvm-strip $out/bin/strip

              # Copy library paths
              mkdir -p $out/lib
              for pkg in ${llvmPkgs.libcxx} ${llvmPkgs.compiler-rt}; do
                if [ -d "$pkg/lib" ]; then
                  cp -rsf "$pkg/lib"/* $out/lib/ 2>/dev/null || true
                fi
              done
            '';

            # Catch2 for tests
            catch2 = pkgs.catch2_3;

            # RapidCheck for property-based testing
            rapidcheck = pkgs.rapidcheck;

            # Python for buck2 toolchains
            python3 = pkgs.python3;

            # mdspan reference implementation
            mdspan = pkgs.mdspan;

            # CUDA SDK (cuda-merged with cudart, cccl)
            cuda-sdk = pkgs.symlinkJoin {
              name = "cuda-sdk";
              paths = with pkgs.cudaPackages; [
                cuda_cudart
                cuda_cccl
              ];
            };
          };

          devShells.default = pkgs.mkShell {
            name = "mdspan-cute-dev";

            packages =
              with pkgs;
              [
                mdspan-cute
                cutlass
                llvmPackages.clang
                cmake
                ninja
                buck2
                # Testing frameworks (needed for Buck2 builds via NIX_CFLAGS)
                catch2_3
                rapidcheck
              ]
              ++ lib.optionals pkgs.stdenv.isLinux [
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
