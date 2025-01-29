{
  pkgs,
  stdenv,
  ruby,
  bundlerEnv,
  ...
}:
let
  gems = bundlerEnv {
    name = "riscv-unified-db-gems";
    inherit ruby;
    gemdir = ./.;
  };
in
stdenv.mkDerivation rec{
  pname = "riscv-unified-db";
  version = "0.0.0";
  src = pkgs.fetchFromGitHub {
    owner = "riscv-software-src";
    repo = "riscv-unified-db";
    rev = "70a4fd0d5c065e06d3c644262cb64a5ec6bb2584";
    hash = "sha256-bVySEQnoPwjSqpB+hTp38OTzKNgA7YmpUs+X0f8NN/g=";
  };

  patches = [ ./riscv-db-skip-setup.patch ];

  buildInputs = [ gems ruby];

  installPhase = ''
      mkdir -p $out/bin
      cp do $out/bin/do
      cp -r bin $out/bin/
    '';
  checkPhase = ''
    ./do test:smoke
  '';
}
