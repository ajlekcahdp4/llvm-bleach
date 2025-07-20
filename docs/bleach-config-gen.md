% BLEACH-CONFIG-GEN(1) bleach-config-gen user manual | Version 0.1.0
% Alexander Romanov
% July 19, 2025

Config generator tool for llvm-bleach

# SYNOPSIS

```sh
bleach-config-gen [OPTIONS]
```

# DESCRIPTION

llvm-bleach comes with predefined set of architecture specifications to use
(Currently only small subset RISC-V and AArch64).
bleach-config-gen is a simple tool to generate bleach config file from provided
templates.

# OPTIONS

**--help**

```
Display available options.
```

**-o, --output \<PATH>**

```
Output file for LLVM IR code.
```

**-t, --template \<FILE>**

```
A template YAML file to generate config from
```

**-d, --template-dir \<DIR>**

```
A path to directory with template files. Can be used instead of --template
```

**--march \<ARCH>**

```
Architecture to generate config for.
```
