# nix/overlays/default.nix
#
# Overlays for mdspan-cute
#
# Adds:
#   - pkgs.cutlass: NVIDIA CUTLASS headers
#   - pkgs.mdspan-cute: This library
#
_inputs:
let
  # Package overlay - adds packages from ../packages/
  packages-overlay = final: _prev: {
    cutlass = final.callPackage ../packages/cutlass.nix { };
    mdspan-cute = final.callPackage ../packages/package.nix {
      inherit (final) cutlass;
    };
  };
in
{
  flake.overlays = {
    packages = packages-overlay;
    default = packages-overlay;
  };
}
