% MCTOMIR(1) bleach mctomir user manual | Version 0.1.0 % Alexander Romanov % July 19, 2025

Machine code to LLVM MIR lifter tool.

# SYNOPSIS

```sh
mctomir ELF_FILE [OPTIONS]
```

# DESCRIPTION

mctomir is a simple tool to lift LLVM MIR code from target ELF file. Lifted MIR code can be used as input file of
llvm-bleach.

# OPTIONS

**--help**

```
Display available options.
```

**-o \<PATH>**

```
Output file for LLVM IR code.
```

**-symtab-file=\<PATH>**

```
File to save YAML-serialized symbol table to. This file can be used later by
llvm-bleach for lifting indirect branches.
```

# EXAMPLES

1. Lift ELF code from ./myfunc.elf and print lifted LLVM MIR on stdout.

```sh
mctomir ./myfunc.elf -o -
```

1. Lift ELF code from ./riscv-foo.elf file, print lifted LLVM MIR to ./riscv-foo.mir.

```sh
mctomir ./riscv-foo.elf -o ./riscv-foo.mir
```

1. Lift ELF code from ./riscv-foo.elf file, print lifted LLVM MIR to ./riscv-foo.mir. Save symbol table to symtab.yaml

```sh
mctomir ./riscv-foo.elf -o ./riscv-foo.mir -symtab-file=symtab.yaml
```
