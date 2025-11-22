#include "riscv-mixed-arithmetic-fpu.h"
#include <STATE>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

typedef struct {
  int i1;
  int i2;
  unsigned u1;
  unsigned u2;
  long l1;
  long l2;
  unsigned long ul1;
  unsigned long ul2;
  float f1;
  float f2;
  float f3;
  float f4;
  double d1;
  double d2;
  double d3;
  double d4;
} arguments;

float random_float_safe(float min, float max) {
  if (min >= max) {
    return (min + max) / 2.0f;
  }

  if (!isfinite(min) || !isfinite(max)) {
    return 0.0f;
  }

  float scale = rand() / (float)RAND_MAX;
  float result = min + scale * (max - min);

  if (isnan(result)) {
    return (min + max) / 2.0f; // Fallback to midpoint
  }
  return result;
}

arguments get_random_arguments() {
  arguments res = {};
  res.i1 = rand();
  res.i2 = rand();
  res.u1 = (unsigned)rand();
  res.u2 = (unsigned)rand();
  res.l1 = ((long)rand() << 31) | rand();
  res.l2 = ((long)rand() << 31) | rand();
  res.ul1 = (unsigned long)(((long)rand() << 31) | rand());
  res.ul2 = (unsigned long)(((long)rand() << 31) | rand());

  res.f1 = random_float_safe(-100, 100);
  res.f2 = random_float_safe(-0.0001, 0.0001);
  res.f3 = random_float_safe(-100, 100);
  res.f4 = random_float_safe(-100, 100);
  res.d1 = random_float_safe(-2, -1);
  res.d2 = random_float_safe(-2, -1);
  res.d3 = random_float_safe(-2, -1);
  res.d4 = random_float_safe(-2, -1);
  return res;
}

int main() {
  printf("Hello from main\n");
  srand(1);
  struct register_state regs = {};
  // random values
  arguments arg_array[] = {
      {42, -123, 100, 200, 1000000, -500000, 3000000, 4000000, 3.1415f,
       2.71828f, 1.41421f, -0.57721f, 123.456, -789.012, 0.001, 1000000.0},
      get_random_arguments(),
  };
  int failed = 0;
  for (unsigned i = 0; i < sizeof(arg_array) / sizeof(arguments); ++i) {
    const arguments *args = &arg_array[i];
    regs.GPR[10] = args->i1;                        // a0
    regs.GPR[11] = args->i2;                        // a1
    regs.GPR[12] = args->u1;                        // a2
    regs.GPR[13] = args->u2;                        // a3
    regs.GPR[14] = args->l1;                        // a4
    regs.GPR[15] = args->l2;                        // a5
    regs.GPR[16] = args->ul1;                       // a6
    regs.GPR[17] = args->ul2;                       // a7
    regs.FPR[10] = bitcast_float_to_int(args->f1);  // fa0 (float)
    regs.FPR[11] = bitcast_float_to_int(args->f2);  // fa1 (float)
    regs.FPR[12] = bitcast_float_to_int(args->f3);  // fa2 (float)
    regs.FPR[13] = bitcast_float_to_int(args->f4);  // fa3 (float)
    regs.FPR[14] = bitcast_double_to_int(args->d1); // fa4 (double)
    regs.FPR[15] = bitcast_double_to_int(args->d2); // fa5 (double)
    regs.FPR[16] = bitcast_double_to_int(args->d3); // fa6 (double)
    regs.FPR[17] = bitcast_double_to_int(args->d4); // fa7 (double)
    double res1 = test_all_operations(args->i1, args->i2, args->u1, args->u2,
                                      args->l1, args->l2, args->ul1, args->ul2,
                                      args->f1, args->f2, args->f3, args->f4,
                                      args->d1, args->d2, args->d3, args->d4);
    bleached_test_all_operations(&regs);

    double res2 = bitcast_to_double(regs.FPR[10]);
    printf("result: %lf\n", res2);
    printf("reference: %lf\n", res1);
    printf("diff: %lf\n", res1 - res2);
    failed |= !are_equal(res1, res2);
  }
  return failed;
}
