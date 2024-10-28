# llvm-bleach

## Fetch all dependencies for the build

You can fetch all the required dependencies using [nix](https://nixos.org/download/)

```sh
nix develop .
```

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run:

```sh
build/bin/llvm-bleach test/tools/llvm-bleach/inputs/foo.mir --instructions test/tools/llvm-bleach/inputs/addsub.yaml
```
