// RUN: clang -fPIC -c %s -o %t.o
// RUN: clang -shared %t.o -o %t.so
// RUN: %bin/mctomir %t.so -o - | FileCheck %s

int foo(int);

int bar(int x) { return foo(x - 1); }

int foo(int x) {
  if (x == 1)
    return 1;
  return x * bar(x);
}

// CHECK: name: bar
// CHECK: body:
// CHECK: bb.0:
// CHECK: CALL
// CHECK: RET64
// CHECK: name: foo
// CHECK: body:
// CHECK: bb.0:
// CHECK: CALL
// CHECK: RET64
