# package.nix - for use with overlays
{ lib
, stdenv
, cutlass
}:

stdenv.mkDerivation {
  pname = "mdspan-cute";
  version = "0.1.0";

  src = ./.;

  dontBuild = true;
  dontConfigure = true;

  installPhase = ''
    runHook preInstall

    mkdir -p $out/include
    cp -r include/* $out/include/

    mkdir -p $out/share/mdspan-cute
    cp -r art $out/share/mdspan-cute/
    cp README.md $out/share/mdspan-cute/

    mkdir -p $out/lib/pkgconfig
    cat > $out/lib/pkgconfig/mdspan-cute.pc << EOF
    prefix=$out
    includedir=\''${prefix}/include

    Name: mdspan-cute
    Description: Bridge between std::mdspan and CUTLASS cute layouts
    Version: 0.1.0
    Cflags: -I\''${includedir}
    EOF

    runHook postInstall
  '';

  propagatedBuildInputs = [ cutlass ];

  meta = with lib; {
    description = "Zero-overhead bridge between std::mdspan and CUTLASS cute layouts";
    longDescription = ''
      mdspan-cute connects C++23 std::mdspan with NVIDIA's CUTLASS cute
      polyhedral tensor algebra. Write tensor[i,j,k] with swizzled layouts,
      hierarchical shapes, and all of cute's composition operations.

      Dedicated to the polyhedral wizards who built the foundations.
    '';
    homepage = "https://github.com/weyl-ai/mdspan-cute";
    license = licenses.asl20;
    platforms = platforms.linux;
  };
}
