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
      nixpkgs,
      flake-parts,
      treefmt-nix,
      ...
    }@inputs:
    let
      systemToMusl = {
        "x86_64-linux" = "x86_64-unknown-linux-musl";
        "aarch64-linux" = "aarch64-unknown-linux-musl";
      };
    in
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ treefmt-nix.flakeModule ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      perSystem =
        { pkgs, system, ... }:
        let
          mkLlvmLib =
            llvmPkgs:
            llvmPkgs.llvm.overrideAttrs (prev: {
              patches = prev.patches ++ [ ./overlays/llvm-install-target-headers.patch ];
              doCheck = false;
              cmakeFlags = prev.cmakeFlags ++ [ "-DLLVM_TARGETS_TO_BUILD=X86;RISCV;AArch64" ];
            });
        in
        rec {
          imports = [ ./nix/treefmt.nix ];

          legacyPackages = {
            bleachPkgsStatic = import nixpkgs {
              localSystem = {
                inherit system;
              };
              crossSystem = {
                inherit system;
                libc = "musl";
                config = systemToMusl.${system};
                isStatic = true;
              };
            };
            bleachPkgs = import nixpkgs { inherit system; };
          };
          packages = rec {
            llvm-bleach = legacyPackages.bleachPkgs.callPackage ./. {
              inherit self;
              llvmLib = mkLlvmLib legacyPackages.bleachPkgs.llvmPackages_19;
              clangCompiler = legacyPackages.bleachPkgs.clang;
            };
            llvm-bleach-static = legacyPackages.bleachPkgsStatic.callPackage ./. {
              llvmLib = mkLlvmLib legacyPackages.bleachPkgsStatic.llvmPackages_19;
              clangCompiler = legacyPackages.bleachPkgs.clang;
            };
            default = llvm-bleach;
          };
          checks = {
            inherit (packages) llvm-bleach;
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
