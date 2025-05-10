// RUN: gcc -c %s -o %t.o -O0
// RUN: %bin/mctomir %t.o -o - | FileCheck %s

extern void foo(int x);

void bar(int x) {
  foo(x);
  foo(42 * x);
}

// CHECK: name: bar
// CHECK: body:
// CHECK-NEXT: bb.0:
// CHECK: CALL
// CHECK: IMUL
// CHECK-SAME: 42
// CHECK: CALL
