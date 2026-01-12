{ lib
, stdenv
, pkgs
, cmake
, mdspan-cute
, cutlass
, cudaPackages
}:

stdenv.mkDerivation {
  pname = "mdspan-cute-example";
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

  # Only build the example target
  buildPhase = ''
    runHook preBuild
    cmake --build . --target swizzled_tile -j$NIX_BUILD_CORES
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp swizzled_tile $out/bin/
    runHook postInstall
  '';

  meta = with lib; {
    description = "Example demonstrating mdspan-cute: C++23 bracket syntax with CUTLASS cute swizzled layouts";
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
