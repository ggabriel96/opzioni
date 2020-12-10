with import <nixpkgs> {};

gcc10Stdenv.mkDerivation {
  name = "opzioni";
  buildInputs = with pkgs; [ conan meson ninja ];
}
