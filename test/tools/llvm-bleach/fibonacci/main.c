#include <stdio.h>

typedef struct {
  long long regs[32];
  long long stack[1000];
} state;

extern long long fibonacci(state *st);

static state regs = {};

static long long lifted_fibonacci(long long n) {
  // According to RISC-V calling convention X10 is the first argument
  regs.regs[10] = n;
  return fibonacci(&regs);
}

void test(long long n) {
  printf("fibonacci(%lld) = %lld\n", n, lifted_fibonacci(n));
}

int main() {
  test(0);
  test(1);
  test(2);
  test(3);
  test(4);
  test(5);
  test(6);
  test(7);
  test(8);
  test(9);
  test(10);
  return 0;
}
