{
  pkgs,
  lib,
  stdenv,
  llvmLib,
  llvm-snippy,
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
    ruby
    clang
    pkgsCross.riscv64.pkgsStatic.stdenv.cc
    pkgsCross.riscv64.buildPackages.clang
    pkgsCross.riscv64.buildPackages.llvmPackages.bintools
    qemu
  ];
  checkInputs = with pkgs; [
    gtest
    llvm-snippy
  ];
  preCheck = ''
    patchShebangs ..
  '';
  doCheck = true;
}
