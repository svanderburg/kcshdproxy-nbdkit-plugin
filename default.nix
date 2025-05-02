with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "kcshdproxy-nbdkit-plugin";
  src = ./.;
  makeFlags = [ "PREFIX=$(out)" ];
  buildInputs = [ nbdkit pkg-config ];
  postBuild = "make searchmbr";
  postInstall = ''
    mkdir -p $out/bin
    cp searchmbr $out/bin
  '';
}
