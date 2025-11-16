// RUN: %config-gen-path/config-gen.rb --march rv64imfd \
// RUN:   --template-dir %config-gen-path/templates -o %t.yaml
// RUN: riscv64-unknown-linux-gnu-clang %s -O2 -c -o %t.out -march=rv64imfd \
// RUN:   -I %S/inputs
// RUN: %bin/llvm-bleach %t.out --instructions %t.yaml -o %t.ll \
// RUN:   --state-struct-file=%t.state.h --lifted-prefix=bleached_
// RUN: sed 's|STATE|%t.state.h|g' %S/inputs/riscv-mixed-fp.c > %t.main.c
// RUN: clang %t.main.c %t.ll -o %t.native.out -g -fsanitize=address,undefined \
// RUN:   -I %S/inputs
// RUN: %t.native.out

#include "riscv-mixed-fp.h"
