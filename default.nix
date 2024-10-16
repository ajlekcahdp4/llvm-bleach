{ pkgs, lib, ... }:
let
  fs = lib.fileset;
in
{
  pname = "llvm-bleach";
  version = "0.0.0";
  src = fs.toSource {
    root = ./.;
    fileset = fs.unions [
      ./CMakeLists.txt
      ./lib
      ./tools
      ./include
      ./test
    ];
  };
  nativeBuildInputs = with pkgs; [
    cmake
    ninja
  ];
  buildInputs = [ ];
}
