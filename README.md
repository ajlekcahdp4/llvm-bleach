# llvm-bleach

llvm-bleach is a tool for lifting LLVM MIR files to LLVM IR. The main feature
of bleach is that it has no backends inside the tool itself. It only uses
platform-independent LLVM MC library information and input configuration files
(that can be modified by user). Currently bleach ships with configuration
files for a subset of RISC-V instructions (rv32/64 with M extension). To add
any other architecture to llvm-bleach one only needs to write a configuration
file describing target platform instructions. No source modification required.

## Build

You can easily build bleach with [nix](https://nixos.org/download/):

```sh
nix build .
```

And run:

```sh
# Generate config file (optional)
result/bin/bleach-config-gen --march rv64imdf --template-dir result/share/templates -o newconfig.yaml
# Run bleach with generated config file
result/bin/llvm-bleach path/to/program.mir --instructions=newconfig.yaml
```

## Developer build

### Fetch all dependencies for the build

You can fetch all the required dependencies using [nix](https://nixos.org/download/)

```sh
nix develop .
```

### Build

```sh
cmake -S . -B build
cmake --build build
```

### Guides

llvm-bleach currently comes with documentation for all of 3 distributed tools:

- [llvm-bleach](./docs/bleach.md) - LLVM MIR to LLVM IR lifter
- [mctomir](./docs/mctomir.md) - machine code to MIR lifter
- [bleach-config-gen](./docs/bleach-config-gen.md) - helper tool to generate
  architecture configs for llvm-bleach from available templates.
