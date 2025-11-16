#include "doubles-mixed.h"
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

int main() {
  printf("Hello from main\n");
  int arg_1 = 3;
  int arg_2 = 4;
  int arg_3 = 5;
  int arg_4 = 3;
  regs.FPR[10] = bitcast_to_int(arg_1);
  regs.FPR[11] = bitcast_to_int(arg_2);
  regs.FPR[12] = bitcast_to_int(arg_3);
  regs.FPR[13] = bitcast_to_int(arg_4);
  bleached_complex_mixed(&regs);
  double res1 = complex_mixed(arg_1, arg_2, arg_3, arg_4);
  double res2 = bitcast_to_double(regs.FPR[10]);
  printf("result: %lf\n", res2);
  printf("reference: %lf\n", res1);
  return res1 - res2 > 0.1;
}
