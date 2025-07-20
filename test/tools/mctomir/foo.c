// RUN: riscv64-unknown-linux-gnu-clang -c %s -o %t.o
// RUN: %bin/mctomir %t.o -o - | FileCheck %s

int foo() { return 42; }

// CHECK: name: foo
// CHECK: body:
// CHECK-NEXT: bb.0:
// CHECK: $x10 = ADDI $x0, 42
// CHECK: C_JR
// CHECK-SAME: $x1
