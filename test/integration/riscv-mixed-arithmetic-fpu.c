// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O1 -o %t.out -march=rv64imfd \
// RUN:   -nostdlib -static -I %S/inputs
// RUN: %bin/llvm-bleach %t.out --instructions %t.yaml -o %t.ll \
// RUN:   --state-struct-file=%t.state.h
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/riscv-mixed-arithmetic-fpu.c > \
// RUN:   %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -g -fsanitize=address,undefined\
// RUN:   -I %S/inputs
// RUN: %t.native.out

#include "riscv-mixed-arithmetic-fpu.h"
