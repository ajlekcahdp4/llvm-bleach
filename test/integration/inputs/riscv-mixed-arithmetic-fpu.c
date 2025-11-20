#include "riscv-mixed-arithmetic-fpu.h"
#include <STATE>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void print(int64_t x) { printf("PRINT CALLED WITH: %lx", x); }

int64_t bitcast_double_to_int(double v) {
  int64_t res;
  memcpy(&res, &v, sizeof(double));
  return res;
}

int64_t bitcast_float_to_int(float v) {
  int64_t res;
  char *ptr = (char *)&res;
  memcpy(ptr, &v, 4);
  unsigned all_ones = 0xffffffff;
  res |= ((uint64_t)all_ones << 32);
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
  printf("Hello from main\n");
  // random values
  // Example calls using your template:
  // GPR arguments (integers)
  struct register_state regs = {};
  int arg_i1 = 42;
  int arg_i2 = -123;
  unsigned arg_u1 = 100;
  unsigned arg_u2 = 200;
  long arg_l1 = 1000000;
  long arg_l2 = -500000;
  unsigned long arg_ul1 = 3000000;
  unsigned long arg_ul2 = 4000000;

  // FPR arguments (floats and doubles)
  float arg_f1 = 3.1415f;
  float arg_f2 = 2.71828f;
  float arg_f3 = 1.41421f;
  float arg_f4 = -0.57721f;
  double arg_d1 = 123.456;
  double arg_d2 = -789.012;
  double arg_d3 = 0.001;
  double arg_d4 = 1000000.0;
  regs.GPR[10] = arg_i1;                        // a0
  regs.GPR[11] = arg_i2;                        // a1
  regs.GPR[12] = arg_u1;                        // a2
  regs.GPR[13] = arg_u2;                        // a3
  regs.GPR[14] = arg_l1;                        // a4
  regs.GPR[15] = arg_l2;                        // a5
  regs.GPR[16] = arg_ul1;                       // a6
  regs.GPR[17] = arg_ul2;                       // a7
  regs.FPR[10] = bitcast_float_to_int(arg_f1);  // fa0 (float)
  regs.FPR[11] = bitcast_float_to_int(arg_f2);  // fa1 (float)
  regs.FPR[12] = bitcast_float_to_int(arg_f3);  // fa2 (float)
  regs.FPR[13] = bitcast_float_to_int(arg_f4);  // fa3 (float)
  regs.FPR[14] = bitcast_double_to_int(arg_d1); // fa4 (double)
  regs.FPR[15] = bitcast_double_to_int(arg_d2); // fa5 (double)
  regs.FPR[16] = bitcast_double_to_int(arg_d3); // fa6 (double)
  regs.FPR[17] = bitcast_double_to_int(arg_d4); // fa7 (double)
  double res1 = test_all_operations(
      arg_i1, arg_i2, arg_u1, arg_u2, arg_l1, arg_l2, arg_ul1, arg_ul2, arg_f1,
      arg_f2, arg_f3, arg_f4, arg_d1, arg_d2, arg_d3, arg_d4);
  bleached_test_all_operations(&regs);

  double res2 = bitcast_to_double(regs.FPR[10]);
  printf("result: %lf\n", res2);
  printf("reference: %lf\n", res1);
  printf("diff: %lf\n", res1 - res2);
  return !are_equal(res1, res2);
}
