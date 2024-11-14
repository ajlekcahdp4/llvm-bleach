{
  pkgs,
  lib,
  stdenv,
  llvmLib,
  ...
}:
let
  fs = lib.fileset;
in
stdenv.mkDerivation {
  pname = "llvm-bleach";
  version = "0.0.0";
  src = fs.toSource {
    root = ./.;
    fileset = fs.unions [
      ./CMakeLists.txt
      ./lib
      ./tools
      ./cmake
      ./include
      ./test
    ];
  };
  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    yaml-cpp
  ];
  buildInputs = [ llvmLib ];
  nativeCheckInputs = with pkgs; [
    lit
    filecheck
    yq
  ];
  checkInputs = with pkgs; [ gtest ];
  doCheck = true;
}
