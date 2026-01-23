# nix/packages/default.nix
#
# Package definitions for mdspan-cute
#
{ pkgs, ... }:
let
  inherit (pkgs) callPackage;

  # Core dependencies
  cutlass = callPackage ./cutlass.nix { };
  mdspan = callPackage ./mdspan.nix { };

  # Main library
  mdspan-cute = callPackage ./package.nix { inherit cutlass; };

  # CUDA SDK (merged headers for proper include paths)
  cuda-sdk = pkgs.symlinkJoin {
    name = "cuda-sdk";
    paths = with pkgs.cudaPackages; [
      cuda_cudart
      cuda_cccl
    ];
  };

  # Example binary
  example = callPackage ./example.nix {
    inherit mdspan-cute cutlass mdspan;
    inherit (pkgs) llvmPackages cudaPackages;
  };
in
{
  inherit
    cutlass
    mdspan
    mdspan-cute
    cuda-sdk
    example
    ;
}
