{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-20.09.tar.gz") {} }:

pkgs.gcc10Stdenv.mkDerivation {
  name = "opzioni";
  buildInputs = with pkgs; [ clang-tools cmake conan meson ninja pkg-config-unwrapped ];
}
