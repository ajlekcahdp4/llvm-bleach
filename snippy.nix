{
  pkgs,
  stdenv,
  lib,
  cmake,
  ninja,
  python3,
  gtest,
  ...
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "llvm-snippy";
  version = "2.0.0";

  src = pkgs.fetchFromGitHub {
    owner = "syntacore";
    repo = "snippy";
    rev = "268b59cd505531645651c3feff1143e599c5b7c2";
    hash = "sha256-A6a8mReDDHdY8JLHSWlK0rwzmALBHohUv/q49UypMc8=";
  };

  sourceRoot = "${finalAttrs.src.name}/llvm";
  strictDeps = true;
  nativeBuildInputs = [
    cmake
    ninja
    python3
  ];
  buildInputs = [ ];

  cmakeFlags = [
    (lib.cmakeFeature "LLVM_SNIPPY_VERSION" finalAttrs.version)
    (lib.cmakeBool "LLVM_ENABLE_RTTI" false)
    (lib.cmakeBool "LLVM_BUILD_SNIPPY" true)
    (lib.cmakeBool "LLVM_ENABLE_ASSERTIONS" true)
    (lib.cmakeBool "LLVM_BUILD_TESTS" finalAttrs.finalPackage.doCheck)
    (lib.cmakeBool "LLVM_INCLUDE_UTILS" finalAttrs.finalPackage.doCheck)
    (lib.cmakeBool "LLVM_INCLUDE_TESTS" finalAttrs.finalPackage.doCheck)
    (lib.cmakeBool "LLVM_INCLUDE_BENCHMARKS" false)
    (lib.cmakeFeature "LLVM_TARGETS_TO_BUILD" "RISCV")
    (lib.cmakeFeature "LLVM_ENABLE_PROJECTS" "lld")
  ];

  installTargets = [ "install-llvm-snippy" ];

  ninjaFlags = [ "llvm-snippy" ];

  doCheck = true;
  nativeCheckInputs = [ ];
  checkInputs = [ gtest ];
  checkTarget = "check-llvm-tools-llvm-snippy";
})
