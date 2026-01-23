# nix/_main.nix
#
# mdspan-cute: Bridge between C++23 std::mdspan and CUTLASS cute layouts
#
# The Villa Straylight Papers
# "Two decades of work, one line of code"
#
{ inputs, ... }:
let
  inherit (inputs) buck2-prelude weyl-std;
in
{
  _class = "flake";

  # Import weyl-std for nixpkgs instantiation (allowUnfree, CUDA, overlays)
  imports = [
    weyl-std.flakeModules.default
  ];

  # Enable NVIDIA support (sets allowUnfree, cudaSupport, cudaCapabilities)
  weyl-std.nixpkgs.cuda.enable = true;

  systems = [
    "x86_64-linux"
    "aarch64-linux"
  ];

  # ════════════════════════════════════════════════════════════════════════════
  # PER-SYSTEM OUTPUTS
  # ════════════════════════════════════════════════════════════════════════════

  perSystem =
    { pkgs, lib, ... }:
    let
      # Import packages
      packages = import ./packages { inherit pkgs; };

      # Import checks
      checks = import ./checks { inherit pkgs; };

      # mdspan with std:: namespace shim
      inherit (packages) mdspan;

      # Import devshells
      devShells = import ./devshells {
        inherit
          pkgs
          lib
          packages
          mdspan
          buck2-prelude
          ;
      };
    in
    {
      # ──────────────────────────────────────────────────────────────────────────
      # PACKAGES
      # ──────────────────────────────────────────────────────────────────────────

      packages = {
        default = packages.mdspan-cute;
        inherit (packages)
          mdspan-cute
          cutlass
          mdspan
          cuda-sdk
          example
          ;

        # Buck2 toolchain exports (for flake.package rules)
        llvm-toolchain = pkgs.runCommand "llvm-toolchain" { } ''
          mkdir -p $out/bin
          for pkg in ${pkgs.llvmPackages_latest.clang} ${pkgs.llvmPackages_latest.lld} ${pkgs.llvmPackages_latest.llvm}; do
            if [ -d "$pkg/bin" ]; then
              for bin in "$pkg/bin"/*; do
                [ -e "$bin" ] && ln -sf "$bin" "$out/bin/$(basename "$bin")"
              done
            fi
          done
          ln -sf ${pkgs.llvmPackages_latest.clang}/bin/clang $out/bin/cc
          ln -sf ${pkgs.llvmPackages_latest.clang}/bin/clang++ $out/bin/c++
          ln -sf ${pkgs.llvmPackages_latest.llvm}/bin/llvm-ar $out/bin/ar
          ln -sf ${pkgs.llvmPackages_latest.llvm}/bin/llvm-nm $out/bin/nm
          ln -sf ${pkgs.llvmPackages_latest.llvm}/bin/llvm-objcopy $out/bin/objcopy
          ln -sf ${pkgs.llvmPackages_latest.llvm}/bin/llvm-ranlib $out/bin/ranlib
          ln -sf ${pkgs.llvmPackages_latest.llvm}/bin/llvm-strip $out/bin/strip
        '';

        catch2 = pkgs.catch2_3;
        inherit (pkgs) rapidcheck python3;
      };

      # ──────────────────────────────────────────────────────────────────────────
      # CHECKS
      # ──────────────────────────────────────────────────────────────────────────

      inherit checks;

      # ──────────────────────────────────────────────────────────────────────────
      # DEVSHELLS
      # ──────────────────────────────────────────────────────────────────────────

      inherit devShells;
    };

  # ════════════════════════════════════════════════════════════════════════════
  # FLAKE-LEVEL OUTPUTS
  # ════════════════════════════════════════════════════════════════════════════

  flake = import ./overlays inputs;
}
