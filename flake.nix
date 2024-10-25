{
  description = "llvm-bleach";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      flake-parts,
      treefmt-nix,
      ...
    }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ treefmt-nix.flakeModule ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      perSystem =
        { pkgs, ... }:
        let
          llvmPkgs = pkgs.llvmPackages_19;
          llvmLib = llvmPkgs.llvm;
          llvmLibDebug = pkgs.enableDebugging (
            (llvmPkgs.llvm.override {
              debugVersion = true;
              enablePolly = false;
            }).overrideAttrs
              (prev: {
                cmakeBuildType = "Debug";
                dontStrip = true;
                debug = true;
                pname = "llvmLibDebug";
                doCheck = false;
                cmakeFlags = prev.cmakeFlags ++ [
                  "-DLLVM_USE_SANITIZER=Address"
                  "-DLLVM_BUILD_TOOLS=OFF"
                ];
              })
          );
        in
        rec {
          imports = [ ./nix/treefmt.nix ];
          packages = rec {
            llvm-bleach = pkgs.callPackage ./. { inherit self llvmLib; };
            default = llvm-bleach;
            inherit llvmLibDebug;
          };
          devShells.default = pkgs.mkShell {
            nativeBuildInputs =
              packages.llvm-bleach.nativeBuildInputs
              ++ (with pkgs; [
                doxygen
                clang-tools
                act
                lldb
                gdb
                valgrind
                just
              ]);
            buildInputs = packages.llvm-bleach.buildInputs;
          };
        };
    };
}
