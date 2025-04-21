// RUN: riscv64-unknown-linux-gnu-clang -c %s -o %t.o
// RUN: %bin/mctomir %t.o -o - | FileCheck %s

int foo() { return 42; }

// CHECK: name: disassembled_function
// CHECK: $x10 = ADDI $x0, 42
// CHECK: PseudoRET
// CHECK-SAME: $x1
