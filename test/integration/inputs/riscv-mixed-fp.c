#include <STATE>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct register_state regs = {};

int64_t bitcast_to_int(double v) {
  int64_t res;
  memcpy(&res, &v, sizeof(double));
  return res;
}

double bitcast_to_double(int64_t v) {
  double res;
  memcpy(&res, &v, sizeof(double));
  return res;
}

// This should be in sync with test/integration/riscv-mixed-fp.c
// TODO: automate this
double ref_calc2(double prod, double sum, int x) {
  return prod + sum - (double)x;
}

double ref_calc1(double a, double b, int x) {
  return ref_calc2(a * b, (a + b), x);
}

double reference(double a, double b, int x) { return ref_calc1(a, b, x); }

int main() {
  printf("Hello from main\n");
  // random values
  double arg_1 = 34.33;
  double arg_2 = 0.0003344;
  int arg_3 = 5;
  regs.FPR[10] = bitcast_to_int(arg_1);
  regs.FPR[11] = bitcast_to_int(arg_2);
  regs.GPR[10] = arg_3;
  top_small(&regs);
  double res1 = reference(arg_1, arg_2, arg_3);
  double res2 = bitcast_to_double(regs.FPR[10]);
  printf("result: %lf\n", res2);
  printf("reference: %lf\n", res1);
  return res1 - res2 > 0.1;
}
