{ lib
, stdenv
, pkgs
, cmake
, mdspan-cute
, cutlass
, cudaPackages
}:

stdenv.mkDerivation {
  pname = "mdspan-cute-tests";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [
    cmake
    stdenv.cc
  ] ++ lib.optionals stdenv.isLinux [ pkgs.git ];


  buildInputs = [
    mdspan-cute
    cutlass
    cudaPackages.cuda_cudart
    cudaPackages.cuda_cccl
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCUTLASS_DIR=${cutlass}"
  ];

  # Build test targets
  buildPhase = ''
    runHook preBuild
    cmake --build . --target layout_cute_tests -j$NIX_BUILD_CORES
    cmake --build . --target property_tests -j$NIX_BUILD_CORES
    runHook postBuild
  '';

  # Run tests during build
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Running mdspan-cute test suite"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    ./layout_cute_tests
    ./property_tests
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp layout_cute_tests $out/bin/
    cp property_tests $out/bin/

    mkdir -p $out/share/doc/mdspan-cute
    cat > $out/share/doc/mdspan-cute/test-results.txt << EOF
    mdspan-cute test suite passed
    - layout_cute_tests: OK
    - property_tests: OK
    EOF
    runHook postInstall
  '';

  meta = with lib; {
    description = "Test suite for mdspan-cute";
    longDescription = ''
      Comprehensive test suite including:
      - layout_cute_tests: Bridge between mdspan and CUTLASS cute layouts
      - property_tests: Property-based tests with RapidCheck
    '';
    homepage = "https://github.com/weyl-ai/mdspan-cute";
    license = licenses.asl20;
    platforms = platforms.linux;
  };
}
