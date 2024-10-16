{
  treefmt = {
    projectRootFile = "flake.nix";
    programs = {
      nixfmt.enable = true;
      yamlfmt.enable = true;
      deadnix.enable = true;
      just.enable = true;
      keep-sorted.enable = true;
      mdformat.enable = true;
      clang-format.enable = true;
      black.enable = true;
      ruff.enable = true;
    };
  };
}
