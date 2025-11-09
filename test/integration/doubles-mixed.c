// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -c -o %t.out -march=rv64imfd
// -nostdlib -static
// RUN: %bin/llvm-bleach %t.out --instructions %t.yaml -o %t.ll \
// RUN:   --state-struct-file=%t.state.h
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/doubles-mixed-main.c > %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -g -fsanitize=address,undefined
// RUN: %t.native.out

double complex_mixed(double a, double b, double c, double d) {
  double part1 = (a + b) * (unsigned)(c - d);
  double part2 = (a * d) + (int)(b * c);
  double part3 = (a / c) * (long long)(b / d);
  return part1 + part2 - part3;
}
