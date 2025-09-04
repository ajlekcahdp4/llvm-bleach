// RUN: %config-gen-path/config-gen.rb --march rv64im \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O1 -o %t.out -march=rv64im -nostdlib
// RUN: %bin/mctomir %t.out -o %t.mir
// RUN: %bin/llvm-bleach %t.mir --instructions %t.yaml \
// RUN:   --state-struct-file %t.state.h > %t.ll
// RUN: clang %S/inputs/bleached-main.c %t.ll -o %t.native.out -I %T -O0
// RUN: %t.native.out | FileCheck %s

// fact.c
__attribute__((noinline)) long long fact(long long num) {
  long long res = 1;
  for (long long i = 2; i <= num; ++i)
    res *= i;
  return res;
}

int main() { return fact(6); }

// CHECK: result: 720
