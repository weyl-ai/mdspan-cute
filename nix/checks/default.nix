# nix/checks/default.nix
#
# Flake checks for mdspan-cute
#
{ pkgs, ... }:
let
  inherit (pkgs) callPackage;
  packages = import ../packages { inherit pkgs; };
  inherit (packages) mdspan;
in
{
  # Test suite - builds and runs all tests
  tests = callPackage ./tests.nix {
    inherit mdspan;
    inherit (packages) mdspan-cute cutlass;
    inherit (pkgs) llvmPackages cudaPackages;
  };
}
