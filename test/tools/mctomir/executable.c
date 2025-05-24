// RUN: gcc %s -o %t.out -O0
// RUN: %bin/mctomir %t.out -o - | FileCheck %s

long long fact(long long x) {
  if (x == 1)
    return 1;
  return x * fact(x - 1);
}

int main() {
  long long x = fact(1);
  x = fact(x);
  x = fact(x);
  x = fact(x);
  x = fact(x);
  return 0;
}

// CHECK: name: main
// CHECK: body:
// CHECK: bb.0:
// CHECK-COUNT-5: CALL
// CHECK: RET64
// CHECK: ...
