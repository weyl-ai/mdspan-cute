{ lib
, stdenv
, fetchFromGitHub
}:

stdenv.mkDerivation rec {
  pname = "cutlass";
  version = "4.3.3";

  src = fetchFromGitHub {
    owner = "NVIDIA";
    repo = "cutlass";
    rev = "v${version}";
    hash = "sha256-uOfSEjbwn/edHEgBikC9wAarn6c6T71ebPg74rv2qlw=";
  };

  dontBuild = true;
  dontConfigure = true;

  installPhase = ''
    runHook preInstall

    mkdir -p $out/include
    cp -r include/cutlass $out/include/
    cp -r include/cute $out/include/

    # Tools and examples for reference
    mkdir -p $out/share/cutlass
    cp -r tools $out/share/cutlass/ || true
    cp -r examples $out/share/cutlass/ || true
    cp -r python $out/share/cutlass/ || true

    echo "${version}" > $out/CUTLASS_VERSION

    runHook postInstall
  '';

  meta = with lib; {
    description = "CUDA Templates for Linear Algebra Subroutines";
    homepage = "https://github.com/NVIDIA/cutlass";
    license = licenses.bsd3;
    platforms = platforms.unix;
  };
}
