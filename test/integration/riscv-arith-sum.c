// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -o %t.out -march=rv64imfd \
// RUN:   -nostdlib -static
// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: %bin/llvm-bleach %t.out --instructions=%t.yaml \
// RUN:   --state-struct-file=%t.state.h > %t.lifted.ll
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/riscv-arith-sum.c > %t.main.c
// RUN: clang %t.main.c %t.lifted.ll -o %t.native.out
// RUN: %t.native.out | FileCheck %s

// CHECK-NOT: FAILED

double arith_seq_sum(double first, double step, double count) {
  double last = first + count * step;
  return count * (first + last);
}
