// RUN: riscv64-unknown-linux-gnu-clang %s -O1 -march=rv64im -nostdlib -o %t.out
// RUN: %bin/mctomir %t.out -o - | FileCheck %s

__attribute__((noinline)) int foo(int x) { return x * 42; }

int main() { return foo(35) + 1; }

// CHECK: JAL @foo
