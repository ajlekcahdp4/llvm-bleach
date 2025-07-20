{
  pkgs,
  lib,
  stdenv,
  cmake,
  which,
  ninja,
  clangCompiler,
  yaml-cpp,
  lit,
  filecheck,
  yq,
  ruby,
  gtest,
  pandoc,
  llvmPackages,
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
      ./docs
      ./version.json
    ];
  };
  nativeBuildInputs = [
    cmake
    ninja
    pandoc
  ];
  buildInputs = [
    llvmLib
    yaml-cpp
  ];
  strictDeps = false;
  enableShared = false;
  nativeCheckInputs =
    [
      lit
      filecheck
      llvmPackages.bintools
      which
      clangCompiler
      yq
      ruby
    ]
    ++ (with pkgs; [
      pkgsCross.riscv64.buildPackages.clang
      pkgsCross.riscv64.buildPackages.llvmPackages.bintools
      pkgsCross.aarch64-multiplatform.buildPackages.clang
    ]);
  preCheck = ''
    patchShebangs ..
  '';
  checkInputs = [ gtest ];
  doCheck = true;
}
