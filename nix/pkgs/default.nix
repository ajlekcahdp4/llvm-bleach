{ pkgs, ... }:
{
  riscv-unified-db = pkgs.callPackage ./riscv-unified-db.nix { };
}
