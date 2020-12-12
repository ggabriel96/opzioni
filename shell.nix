with import <nixpkgs> {};

gcc10Stdenv.mkDerivation {
  name = "opzioni";
  buildInputs = with pkgs; [ clang-tools cmake conan meson ninja pkg-config-unwrapped ];
}
