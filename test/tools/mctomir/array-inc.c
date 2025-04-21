// RUN: riscv64-unknown-linux-gnu-clang -O2 -march=rv64im -c %s -o %t.o
// RUN: %bin/mctomir %t.o -o - | FileCheck %s

int array_inc() {
  int arr[100] = {};
  for (int i = 0; i < 100; ++i)
    arr[i] += i;
  int acc = 0;
  for (int i = 0; i < 100; ++i)
    acc += arr[i];
  return acc;
}

// CHECK: bb.[[HEADER:[0-9]+]]:
// CHECK: bb.[[BODY:[0-9]+]]:
// CHECK: bb.[[EXIT:[0-9]+]]:
// CHECK: BNE
// CHECK-SAME: [[HEADER]]
