// RUN: riscv64-unknown-linux-gnu-clang -c -O2 %S/inputs/single-mul-func.c \
// RUN:   -march=rv64im -o %t.o
// RUN: %bin/mctomir %t.o -o %t.mir
// RUN: %config-gen-path/config-gen.rb --march rv64im \
// RUN:   -d %config-gen-path/templates -o %t.yaml
// RUN: %bin/llvm-bleach %t.mir --instructions %t.yaml \
// RUN:   --state-struct-file=%T/state.h -march=riscv64 > %t.ll
// RUN: clang -c %s -I %T -o %t.main.o
// RUN: clang -c %t.ll -o %t.foo.o
// RUN: clang %t.foo.o %t.main.o -o %t.out
// RUN: %t.out | FileCheck %s

// CHECK: foo(0) = 0
// CHECK: foo(1) = 42
// CHECK: foo(2) = 84
// CHECK: foo(3) = 126

#include <stdio.h>
// generated header
#include <state.h>

extern void foo(struct register_state *st);

static struct register_state regs = {};

static long long lifted_foo(long long n) {
  // According to RISC-V calling convention X10 is the first argument
  regs.GPR[10] = n;
  foo(&regs);
  return regs.GPR[10];
}

void test(long long n) { printf("foo(%lld) = %lld\n", n, lifted_foo(n)); }

int main() {
  test(0);
  test(1);
  test(2);
  test(3);
  return 0;
}
