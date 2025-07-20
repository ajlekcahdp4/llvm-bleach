% MCTOMIR(1) bleach mctomir user manual | Version 0.1.0
% Alexander Romanov
% July 19, 2025

Machine code to LLVM MIR lifter tool.

# SYNOPSIS

```sh
mctomir ELF_FILE [OPTIONS]
```

# DESCRIPTION

mctomir is a simple tool to lift LLVM MIR code from target ELF file. Lifted
MIR code can be used as input file of llvm-bleach.

# OPTIONS

**--help**

```
Display available options.
```

**-o \<PATH>**

```
Output file for LLVM IR code.
```

# EXAMPLES

1. Lift ELF code from ./myfunc.elf and print lifted LLVM MIR on stdout.

```sh
mctomir ./myfunc.elf -o -
```

1. Lift ELF code from ./riscv-foo.elf file, print lifted LLVM MIR to
   ./riscv-foo.mir. Match return instructions.

```sh
mctomir ./riscv-foo.elf -o ./riscv-foo.mir
```
