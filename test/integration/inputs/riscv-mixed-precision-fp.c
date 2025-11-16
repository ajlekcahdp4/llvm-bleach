#include "riscv-mixed-precision-fp.h"
#include <STATE>

#include <math.h>
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

int are_equal(double a, double b) {
  if (isnan(a) && isnan(b))
    return 1;
  if (isnan(a) || isnan(b))
    return 0;
  return fabs(a - b) <= 0.1;
}

int main() {
  // random values that cause difference between computation in floats and
  // doubles
  double arg_1 = 4113444444434.33;
  double arg_2 = 0.0000003344;
  int arg_3 = 5;
  regs.FPR[10] = bitcast_to_int(arg_1);
  regs.FPR[11] = bitcast_to_int(arg_2);
  regs.GPR[10] = arg_3;
  bleached_top_small(&regs);
  double res1 = top_small(arg_1, arg_2, arg_3);
  double res2 = bitcast_to_double(regs.FPR[10]);
  printf("result: %lf\n", res2);
  printf("reference: %lf\n", res1);
  printf("diff: %lf\n", res1 - res2);
  return !are_equal(res1, res2);
}
