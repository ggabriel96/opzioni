with import <nixpkgs> {};

gcc10Stdenv.mkDerivation {
  name = "opzioni";
  buildInputs = with pkgs; [ clang_11 cmake conan meson ninja pkg-config-unwrapped ];
}
