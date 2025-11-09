// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -c -o %t.out -march=rv64imfd
// RUN: %bin/llvm-bleach %t.out --instructions %t.yaml -o %t.ll \
// RUN:   --state-struct-file=%t.state.h
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/riscv-mixed-fp.c > %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -g -fsanitize=address,undefined
// RUN: %t.native.out

double calc2(double prod, double sum, int x) { return prod + sum - (double)x; }

double calc1(double a, double b, int x) { return calc2(a * b, (a + b), x); }

double top_small(double a, double b, int x) { return calc1(a, b, x); }
