{
  description = "Buck2 toolchain flake for mdspan-cute";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      inherit (nixpkgs) lib;
      defaultSystems = [
        "aarch64-linux"
        "x86_64-linux"
      ];
      forAllSystems =
        fn:
        lib.genAttrs defaultSystems (
          system:
          fn (
            import nixpkgs {
              inherit system;
              config.allowUnfree = true;
              config.cudaSupport = true;
            }
          )
        );

      # CUTLASS headers-only package
      cutlassPackage =
        {
          lib,
          stdenv,
          fetchFromGitHub,
        }:
        stdenv.mkDerivation rec {
          pname = "cutlass";
          version = "4.3.3";

          src = fetchFromGitHub {
            owner = "NVIDIA";
            repo = "cutlass";
            rev = "v${version}";
            hash = "sha256-uOfSEjbwn/edHEgBikC9wAarn6c6T71ebPg74rv2qlw=";
          };

          dontBuild = true;
          dontConfigure = true;

          installPhase = ''
            runHook preInstall
            mkdir -p $out/include
            cp -r include/cutlass $out/include/
            cp -r include/cute $out/include/
            runHook postInstall
          '';

          meta = with lib; {
            description = "CUDA Templates for Linear Algebra Subroutines";
            homepage = "https://github.com/NVIDIA/cutlass";
            license = licenses.bsd3;
          };
        };

      # mdspan reference implementation
      mdspanPackage =
        {
          lib,
          stdenv,
          fetchFromGitHub,
          cmake,
        }:
        stdenv.mkDerivation rec {
          pname = "mdspan";
          version = "0.6.0";

          src = fetchFromGitHub {
            owner = "kokkos";
            repo = "mdspan";
            rev = "mdspan-${version}";
            hash = "sha256-bwE+NO/n9XsWOp3GjgLHz3s0JR0CzNDernfLHVqU9Z8=";
          };

          nativeBuildInputs = [ cmake ];
          cmakeFlags = [
            "-DMDSPAN_ENABLE_TESTS=OFF"
            "-DMDSPAN_ENABLE_EXAMPLES=OFF"
            "-DMDSPAN_ENABLE_BENCHMARKS=OFF"
          ];

          meta = with lib; {
            description = "Reference implementation of P0009 std::mdspan";
            homepage = "https://github.com/kokkos/mdspan";
            license = with licenses; [
              asl20
              bsd3
            ];
          };
        };
    in
    {
      packages = forAllSystems (pkgs: {
        inherit (pkgs) python3;

        # Catch2 and RapidCheck for testing
        catch2 = pkgs.catch2_3;
        rapidcheck = pkgs.rapidcheck;

        # CUTLASS headers
        cutlass = pkgs.callPackage cutlassPackage { };

        # mdspan reference implementation
        mdspan = pkgs.callPackage mdspanPackage { };

        # CUDA SDK
        cuda-sdk = pkgs.symlinkJoin {
          name = "cuda-sdk";
          paths = with pkgs.cudaPackages; [
            cuda_cudart
            cuda_cccl
          ];
        };

        # C++ toolchain with wrapped compilers that capture NIX_CFLAGS_COMPILE
        # This bakes in include paths for catch2, rapidcheck, cutlass, etc.
        cxx =
          let
            llvmPkgs = pkgs.llvmPackages_latest;
            cutlass = pkgs.callPackage cutlassPackage { };
            mdspan = pkgs.callPackage mdspanPackage { };
          in
          pkgs.stdenv.mkDerivation {
            name = "buck2-cxx-mdspan-cute";
            dontUnpack = true;
            dontCheck = true;

            nativeBuildInputs = [ pkgs.makeWrapper ];

            # These packages' include paths will be captured via NIX_CFLAGS_COMPILE
            buildInputs = [
              pkgs.catch2_3
              pkgs.rapidcheck
              cutlass
              mdspan
              pkgs.cudaPackages.cuda_cudart
              pkgs.cudaPackages.cuda_cccl
              pkgs.cudaPackages.cuda_nvcc
            ];

            buildPhase = ''
              function capture_env() {
                  # variables to export, all variables with names beginning with one of these are exported
                  local -ar vars=(
                      NIX_CC_WRAPPER_TARGET_HOST_
                      NIX_CFLAGS_COMPILE
                      NIX_DONT_SET_RPATH
                      NIX_ENFORCE_NO_NATIVE
                      NIX_HARDENING_ENABLE
                      NIX_IGNORE_LD_THROUGH_GCC
                      NIX_LDFLAGS
                      NIX_NO_SELF_RPATH
                  )
                  for prefix in "''${vars[@]}"; do
                      for v in $( eval 'echo "''${!'"$prefix"'@}"' ); do
                          echo "--set"
                          echo "$v"
                          echo "''${!v}"
                      done
                  done
              }

              mkdir -p "$out/bin"

              # Link LLVM tools
              for tool in ar nm objcopy ranlib strip; do
                  ln -sf "${llvmPkgs.llvm}/bin/llvm-$tool" "$out/bin/$tool"
              done

              mapfile -t < <(capture_env)

              # Wrap clang/clang++ with captured environment
              makeWrapper "${llvmPkgs.clang}/bin/clang" "$out/bin/cc" "''${MAPFILE[@]}"
              makeWrapper "${llvmPkgs.clang}/bin/clang++" "$out/bin/c++" "''${MAPFILE[@]}"

              # Also create lib directory with libc++ for linking
              mkdir -p "$out/lib"
              for pkg in ${llvmPkgs.libcxx} ${llvmPkgs.compiler-rt}; do
                if [ -d "$pkg/lib" ]; then
                  cp -rsf "$pkg/lib"/* "$out/lib/" 2>/dev/null || true
                fi
              done
            '';
          };
      });
    };
}
