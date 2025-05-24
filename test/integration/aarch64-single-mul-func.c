// RUN: aarch64-unknown-linux-gnu-clang -c -O0 %S/inputs/single-long-mul-func.c \
// RUN:   -o %t.o
// RUN: %bin/mctomir %t.o -o %t.mir
// RUN: %config-gen-path/config-gen.rb --march aarch64 \
// RUN:   -d %config-gen-path/templates -o %t.yaml
// RUN: %bin/llvm-bleach %t.mir --instructions %t.yaml -noinline-instr \
// RUN:   --state-struct-file=%t.state.h > %t.ll
// RUN: sed 's|STATE_FILE|%t.state.h|g' %s > %t.main.c
// RUN: clang -c %t.main.c -I %T -o %t.main.o
// RUN: clang -c %t.ll -o %t.foo.o
// RUN: clang %t.foo.o %t.main.o -o %t.out
// RUN: %t.out | FileCheck %s

// CHECK: foo(0) = 0
// CHECK: foo(1) = 42
// CHECK: foo(2) = 84
// CHECK: foo(3) = 126

#include <stdint.h>
#include <stdio.h>
// generated header
#include <STATE_FILE>

int64_t NZCV = 0;
// dummy implementations
void store_dw(int64_t, int64_t) {}
int64_t load_dw(int64_t) { return 0xff; }

extern void foo(struct register_state *st);

static struct register_state regs = {};

static long long lifted_foo(long long n) {
  // According to AArch64 ABI X0 is the first argument
  // FIXME: here we abuse that in llvm's GPR regclass X0's index is 4
  regs.GPR64[4] = n;
  foo(&regs);
  return regs.GPR64[4];
}

void test(long long n) { printf("foo(%lld) = %lld\n", n, lifted_foo(n)); }

int main() {
  test(0);
  test(1);
  test(2);
  test(3);
  return 0;
}
