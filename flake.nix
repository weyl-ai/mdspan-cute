{
  description = "mdspan-cute: Bridge between C++23 std::mdspan and CUTLASS cute layouts";

  inputs = {
    weyl-std.url = "github:weyl-ai/weyl-std/dev";
    nixpkgs.follows = "weyl-std/nixpkgs";
    flake-parts.follows = "weyl-std/flake-parts";

    # Buck2 prelude (non-flake, just the source)
    buck2-prelude = {
      url = "github:weyl-ai/straylight-buck2-prelude/dev";
      flake = false;
    };
  };

  outputs =
    inputs@{ flake-parts, ... }: flake-parts.lib.mkFlake { inherit inputs; } (import ./nix/_main.nix);
}
