% LLVM-BLEACH(1) bleach user manual | Version 0.1.0
% Alexander Romanov
% July 19, 2025

Highly configurable lifter to LLVM IR.

# SYNOPSIS

```sh
llvm-bleach MIR_FILE --instructions=YAML_PATH [OTHER OPTIONS]
```

# DESCRIPTION

llvm-bleach is a tool to lift LLVM MIR code to target-independent LLVM IR to
analyze, modify or recompile to any other architecture. Input LLVM MIR can be
lifted from ELF file with **mctomir** tool. llvm-bleach is written with the
philosophy that lifter should be as generic as possible and contain no
architecture-specific code as it highly increases cost of maintanence and makes
supporting new source architectures more difficult. Instead bleach uses
target-independent concepts and algorithms. Target description is received from
LLVM backend as well as input architecture description.

# OPTIONS

**--help**

```
Display available options.
```

**-o \<PATH>**

```
Output file for LLVM IR code.
```

**--instructions=\<path>**

```
File containing YAML description of the architecture to lift from.
I.e. description of RISC-V subset that is used in source program.
```

**--noinline-instr**

```
Do not inline function calls generated for each instruction.
```

**--stack-size=\<NUMBER>**

```
The size of inline stack (in bytes) to use. Inline stack is a memory region
that is used to simulate program stack of the source program. Default is
8000. Set this to high enough number for your program. Stack overflows are
not currently diagnosed.
```

**--state-struct-file=\<PATH>**

```
Path of the file to save state C struct definition in. State struct is a
class that is used by lifted LLVM IR code to store current values of source
program registers as well as inline stack. State struct is necessary to
recompile lifted program to other architecture. Bleach generates no struct
definition by default.
```

# EXAMPLES

1. Lift MIR code from ./myfunc.mir file using instruction description from
   ./riscv.yaml file. Print lifted LLVM IR on stdout.

   ```sh
   llvm-bleach ./myfunc.mir --instructions riscv.yaml -o -
   ```

1. Same but also save state struct definition to `./state.h`

   ```sh
   llvm-bleach ./myfunc.mir --instructions riscv.yaml --state-struct-file=./state.h -o -
   ```

1. Lift MIR code from ./myfunc.mir file using instruction description from
   ./instrs.yaml file. Print lifted LLVM IR to ./myfunc.ll

   ```sh
   llvm-bleach ./myfunc.mir --instructions instrs.yaml -o ./myfunc.ll
   ```

1. Same but increase inline stack size to 16000 bytes

   ```sh
   llvm-bleach ./myfunc.mir --instructions instrs.yaml -o ./myfunc.ll --stack-size 16000
   ```

# YAML ARCHITECHURE DESCRIPTION

Architecture description of source program architecture consists of several
top-level keys.

- **register-classes**

  A mapping from custom register class names (used only as a member names in
  state struct definition) to regular expressions matching register names to
  put in this class. Register names are the same as in LLVM backend for
  selected architecture (e.g. X0-X31 for RISC-V or EAX, EDX, R16-R31 for X86).

  Example for RISC-V with I and F extensions:

```yaml
    ---
    register-classes:
      GPR: "X[0-9][0-9]?"
      FPR: "F[0-9][0-9]?_F"
    ...
```

```
Entries for all used register classes are required but no others. I.e. if
your are lifting a program compiled for rv64imf but program does not use any
floating point registers user doesn't have to specify FPR register class.
Though it is not forbidden
```

- **instructions**

  List of instruction definitions. Each instruction definition consists of
  LLVM IR function with the same name as an instruction in LLVM backend.
  For most instructions every input operand becomes an argument of this
  function and destination operand becomes return value.

  Example for RISC-V ADD and SUB instructions:

```yaml
---
instructions:
  - ADD:
      func: |
        define i\xlen\ @ADD(i\xlen\ noundef signext %0, i\xlen\ noundef signext %1) {
          %3 = add i\xlen\ %1, %0
          ret i\xlen\ %3
        }
  - SUB:
      func: |
        define i\xlen\ @SUB(i\xlen\ noundef signext %0, i\xlen\ noundef signext %1) {
          %3 = sub i\xlen\ %0, %1
          ret i\xlen\ %3
        }
...
```

```
This list should contain entries for all instructions that are used in the
source program but no other instructions are required. 
```

- **constant-registers**

  This key is used to specify constant registers of source architecture
  (e.g. X0 for RISC-V or XZR for AArch64). It consists of mappings from
  constant register names to their values.

  Example for XZR AArch64 register:

```yaml
---
constant-registers:
  XZR: 0x0000000000000000
...
```

- **stack-pointer**

  Put the name of stack pointer register under this key. It is used to detect
  that memory instruction works with program stack and not the heap.

  Example for AArch64:

```yaml
---
stack-pointer: SP
...
```

```
Example for RISC-V:
```

```yaml
---
stack-pointer: X2
...
```

- **extern-functions**

  A list of globals **declarations** (functions or global variables ) that
  are used by any of specified instructions. External declarations are a way
  to insert callbacks into your program, emulate CSRs or capture memory
  accesses. You use these definitions in your instruction descriptions. E.g.
  call external `load_double_word` function from load instruction description
  or insert a callback that logs register values in every instruction.

  Example for AArch64 with definitions of `store_dw` and `load_dw` functions
  as well as global `NZCV` flags register:

```yaml
---
extern-functions:
  - "@NZCV = external global i64"
  - "declare void @store_dw(i64 %val, i64 %addr)"
  - "declare i64 @load_dw(i64 %addr)"

instructions:
  - LDRXui:
      func: |
        define i64 @LDRXui(i64 %base, i64 %off) {
          %addr = add i64 %base, %off
          %val = call i64 @load_dw(i64 %val, i64 %addr)
          ret i64 %val
        }
...
```

# State Struct Definition

Here's an example of generated state struct definition for architecture with
stack size equal to 8000 bytes and the following register classes:

Register classes:

```yaml
---
register-classes:
  GPR: "X[0-9][0-9]?"
...
```

Generated state struct definition:

```C
#include <stdint.h>
struct register_state {
  int64_t GPR[32];
  int64_t stack[1000];
};
```
