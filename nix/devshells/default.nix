# nix/devshells/default.nix
#
# Development shells for mdspan-cute
#
{ pkgs
, lib
, packages
, mdspan
, buck2-prelude
,
}:
{
  # Main development shell
  default = pkgs.mkShell {
    name = "mdspan-cute-dev";
    packages =
      with pkgs;
      [
        packages.mdspan-cute
        packages.cutlass
        llvmPackages_latest.clang
        cmake
        ninja
        buck2
        catch2_3
        rapidcheck
        mdspan
        elan
      ]
      ++ lib.optionals stdenv.isLinux [
        cudaPackages.cuda_nvcc
        cudaPackages.cuda_cudart
        cudaPackages.cuda_cccl
      ];

    shellHook = ''
      # Set up Buck2 prelude symlink
      if [ ! -L prelude ] || [ "$(readlink prelude)" != "${buck2-prelude}" ]; then
        rm -rf prelude
        ln -sf ${buck2-prelude} prelude
        echo "  Linked prelude -> ${buck2-prelude}"
      fi

      # Generate .buckconfig.local with hermetic Nix paths
      cat > .buckconfig.local << EOF
      [cxx]
      mdspan_include = ${mdspan}/include
      EOF
      echo "  Generated .buckconfig.local"

      # Set up Lean toolchain for proof compilation
      if [ -f src/lean4-proof/lean-toolchain ]; then
        export ELAN_HOME="''${ELAN_HOME:-$HOME/.elan}"
        LEAN_TOOLCHAIN=$(cat src/lean4-proof/lean-toolchain)
        if ! elan show 2>/dev/null | grep -q "$LEAN_TOOLCHAIN"; then
          echo "  Installing Lean toolchain: $LEAN_TOOLCHAIN"
          elan toolchain install "$LEAN_TOOLCHAIN" 2>/dev/null || true
        fi
        elan default "$LEAN_TOOLCHAIN" 2>/dev/null || true
      fi

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
  lean = pkgs.mkShell {
    name = "villa-straylight-lean";

    packages = with pkgs; [
      elan
      git
      curl
      gmp
    ];

    shellHook = ''
      echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
      echo "  Villa Straylight proof development shell"
      echo "  Lean 4 + Mathlib"
      echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
      echo ""
      echo "  cd src/lean4-proof"
      echo "  lake exe cache get   # download Mathlib oleans"
      echo "  lake build           # build VillaStraylight"
      echo ""

      if [ -f src/lean4-proof/lean-toolchain ]; then
        cd src/lean4-proof && elan override set "$(cat lean-toolchain)" && cd ../..
      fi
    '';

    ELAN_HOME = "$HOME/.elan";
  };
}
