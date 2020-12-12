with import <nixpkgs> {};

gcc10Stdenv.mkDerivation {
  name = "opzioni";
  buildInputs = with pkgs; [ cmake conan meson ninja pkg-config-unwrapped ];
}
