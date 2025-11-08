// RUN: %config-gen-path/config-gen.rb --march rv64im \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -o %t.out -march=rv64im -nostdlib
// RUN: %bin/mctomir %t.out -o %t.mir
// RUN: sed -i 's/$x0 = JAL /PseudoBR /g' %t.mir
// RUN: %bin/llvm-bleach %t.mir --instructions %t.yaml \
// RUN:   --state-struct-file %t.state.h > %t.ll
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/bleached-main-local-var.c > \
// RUN:   %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -O2
// RUN: %t.native.out | FileCheck %s

// fact.c
__attribute__((noinline)) unsigned long long fact(unsigned long long num) {
  unsigned long long res = 1;
  for (unsigned long long i = 2; i <= num; ++i)
    res *= i;
  return res;
}

int main() { return fact(5); }

// CHECK: result: 120
