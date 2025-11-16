// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -c -o %t.out -march=rv64imfd
// RUN: %bin/llvm-bleach %t.out --instructions %t.yaml -o %t.ll \
// RUN:   --state-struct-file=%t.state.h
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/riscv-mixed-precision-fp.c > \
// RUN:   %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -g \
// RUN:   -fsanitize=address,undefined
// RUN: %t.native.out

double calc2(float prod, double sum, int x) { return prod + sum - (double)x; }

double calc1(float a, float b, int x) {
  return calc2(a * b, (a + b), x + a / b);
}

double top_small(double a, double b, int x) { return calc1(a, b, x); }
