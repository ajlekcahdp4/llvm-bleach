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

int main() {
  printf("factorial(5) = %lld\n", lifted_factorial(5));
  return 0;
}
