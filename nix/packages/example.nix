# nix/packages/example.nix
#
# Build swizzled_tile example
# Run with: nix run .#example
#
{ lib
, stdenv
, llvmPackages
, cutlass
, mdspan
, mdspan-cute
, cudaPackages
,
}:

stdenv.mkDerivation {
  pname = "mdspan-cute-example";
  version = "0.1.0";

  src = lib.cleanSourceWith {
    src = ../..;
    filter =
      path: _type:
      let
        baseName = baseNameOf path;
        relPath = lib.removePrefix (toString ../.. + "/") path;
      in
      # Exclude .lake, buck-out, prelude symlink, and git
      !(lib.hasPrefix "src/lean4-proof/.lake" relPath)
      && !(lib.hasPrefix "buck-out" relPath)
      && !(lib.hasPrefix ".git" relPath)
      && !(lib.hasPrefix "nix" relPath)
      && baseName != "prelude"
      && baseName != "result";
  };

  nativeBuildInputs = [
    llvmPackages.clang
  ];

  buildInputs = [
    cutlass
    mdspan
    mdspan-cute
    cudaPackages.cudatoolkit
  ];

  # Direct compilation (Buck2 doesn't work in Nix sandbox)
  buildPhase = ''
    runHook preBuild

    mkdir -p $out/bin
    clang++ $NIX_CFLAGS_COMPILE -std=c++23 -O2 \
      -I src/mdspan-cute/include \
      src/mdspan-cute/examples/swizzled_tile.cpp \
      -o $out/bin/swizzled_tile

    runHook postBuild
  '';

  dontInstall = true;

  meta = with lib; {
    description = "Example: C++23 bracket syntax with CUTLASS cute swizzled layouts";
    longDescription = ''
      Two decades of work, one line of code:
        tile[row, col] = value;

      Feautrier's polyhedra, Kerr's swizzles, Lelbach's syntax.
    '';
    homepage = "https://github.com/weyl-ai/mdspan-cute";
    license = licenses.asl20;
    platforms = platforms.linux;
    mainProgram = "swizzled_tile";
  };
}
