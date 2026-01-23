# nix/checks/tests.nix
#
# Build and run mdspan-cute test suite
# Direct clang++ compilation (Buck2 doesn't work in Nix sandbox)
#
{ lib
, stdenv
, llvmPackages
, catch2_3
, rapidcheck
, cutlass
, mdspan
, mdspan-cute
, cudaPackages
,
}:

stdenv.mkDerivation {
  pname = "mdspan-cute-tests";
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
    catch2_3
    rapidcheck
    cutlass
    mdspan
    mdspan-cute
    cudaPackages.cudatoolkit
  ];

  buildPhase = ''
    runHook preBuild

    mkdir -p $out/bin

    echo "Building layout_cute_tests..."
    clang++ $NIX_CFLAGS_COMPILE -std=c++23 -O2 \
      -I src/mdspan-cute/include \
      src/mdspan-cute/tests/test_layout_cute.cpp \
      -lCatch2Main -lCatch2 -lrapidcheck \
      -o $out/bin/layout_cute_tests

    echo "Building property_tests..."
    clang++ $NIX_CFLAGS_COMPILE -std=c++23 -O2 \
      -I src/mdspan-cute/include \
      src/mdspan-cute/tests/property_tests.cpp \
      -lCatch2Main -lCatch2 -lrapidcheck \
      -o $out/bin/property_tests

    runHook postBuild
  '';

  doCheck = true;
  checkPhase = ''
    runHook preCheck
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Running mdspan-cute test suite"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    echo "Running layout tests..."
    $out/bin/layout_cute_tests

    echo "Running property tests..."
    $out/bin/property_tests

    runHook postCheck
  '';

  # Binaries already in $out/bin from buildPhase
  dontInstall = true;

  meta = with lib; {
    description = "Test suite for mdspan-cute";
    homepage = "https://github.com/weyl-ai/mdspan-cute";
    license = licenses.asl20;
    platforms = platforms.linux;
  };
}
