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
      flake.overlays.default = _final: prev: {
        llvmPackages_19.libllvm = prev.llvmPackages_19.libllvm.overrideAttrs {
          patches = prev.patches ++ [ ./overlays/llvm-install-target-headers.patch ];
        };
      };
      perSystem =
        { pkgs, ... }:
        let
          llvmPkgs = pkgs.llvmPackages_19;
          llvmLib = llvmPkgs.llvm.overrideAttrs (prev: {
            patches = prev.patches ++ [ ./overlays/llvm-install-target-headers.patch ];
          });
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
            riscv-unified-db = pkgs.callPackage ./nix/pkgs/riscv-unified-db.nix { };
          };
          devShells.default = pkgs.mkShell {
            nativeBuildInputs =
              packages.llvm-bleach.nativeBuildInputs
              ++ (with pkgs; [
                doxygen
                clang-tools
                lit
                filecheck
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
