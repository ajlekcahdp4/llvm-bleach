// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -o %t
// RUN: %bin/mctomir %t -o - | FileCheck %s

// CHECK: name: fact
// CHECK: bb.0:
// CHECK-NEXT: successors:
// CHECK-SAME: %bb.2
// CHECK: BLT
// CHECK-SAME: %bb.2

int fact(int x) {
  int res = 1;
  while (x > 1)
    res *= x;
  return res;
}
int main() { return fact(10); }
