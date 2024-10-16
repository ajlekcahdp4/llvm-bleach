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
    { flake-parts, treefmt-nix, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ treefmt-nix.flakeModule ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      perSystem =
        { pkgs, ... }:
        rec {
          imports = [ ./nix/treefmt.nix ];
          packages = rec {
            llvm-bleach = import ./. { };
            default = llvm-bleach;
          };
          devShells.default = pkgs.mkShell {
            nativeBuildInputs =
              packages.llvm-bleach.nativeBuildInputs
              ++ (with pkgs; [
                doxygen
                clang-tools
                act
                just
              ]);
          };
        };
    };
}
