#include <stdio.h>
// generated header
#include <state.h>

static struct register_state regs = {};

static long long lifted_factorial(long long n) {
  // According to RISC-V calling convention X10 is the first argument
  regs.GPR[10] = n;
  bleached_fact(&regs);
  return regs.GPR[10];
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
