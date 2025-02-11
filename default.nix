{
  pkgs,
  lib,
  stdenv,
  llvmLib,
  ...
}:
let
  fs = lib.fileset;
  versionJson = builtins.fromJSON (builtins.readFile ./version.json);
in
stdenv.mkDerivation {
  pname = "llvm-bleach";
  version = versionJson.version;
  src = fs.toSource {
    root = ./.;
    fileset = fs.unions [
      ./CMakeLists.txt
      ./lib
      ./tools
      ./cmake
      ./include
      ./test
      ./version.json
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
    clang
    pkgsCross.riscv64.buildPackages.clang
    pkgsCross.riscv64.buildPackages.llvmPackages.bintools
  ];
  checkInputs = with pkgs; [ gtest ];
  doCheck = true;
}
