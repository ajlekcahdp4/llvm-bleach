#include <stdio.h>

typedef struct {
  long long regs[32];
  long long stack[1000];
} state;

extern long long fact(state *st);

static state regs = {};

static long long lifted_factorial(long long n) {
  // According to RISC-V calling convention X10 is the first argument
  regs.regs[10] = n;
  return fact(&regs);
}

void test(long long n) {
  printf("factorial(%lld) = %lld\n", n, lifted_factorial(n));
}

int main() {
  test(5);
  test(0);
  test(1);
  test(13);
  return 0;
}
