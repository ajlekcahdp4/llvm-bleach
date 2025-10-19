// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -o %t -nostdlib
// RUN: %bin/mctomir %t -o - | FileCheck %s

// CHECK: name: fact
// CHECK: bb.0:
// CHECK-NEXT: successors:
// CHECK-SAME: %bb.[[REG:[0-9]]]
// CHECK: BLT
// CHECK-SAME: %bb.[[REG]]

int fact(int x) {
  int res = 1;
  while (x > 1)
    res *= x--;
  return res;
}
int main() { return fact(6); }
