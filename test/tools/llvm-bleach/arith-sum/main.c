#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// generated header
#include <state.h>

extern double arith_seq_sum(struct register_state *st);

static struct register_state regs = {};

double uitod(int64_t n) {
  double res = 0.0;
  memcpy(&res, &n, sizeof(res));
  return res;
}

int64_t dtosi(double n) {
  int64_t res = 0;
  memcpy(&res, &n, sizeof(res));
  return res;
}

static double lifted_arith_seq_sum(double first, double step, uint64_t count) {
  // According to RISC-V calling convention X10 is the first GPR argument
  // and F10-12 - three first FPR arguments
  regs.FPR[10] = dtosi(first);
  regs.FPR[11] = dtosi(step);
  regs.FPR[12] = dtosi(count);
  regs.GPR[10] = 1;
  arith_seq_sum(&regs);
  return uitod(regs.FPR[10]) / 2;
}

double sum(double first, double step, uint64_t count) {
  return count * (2 * first + step * count) / 2;
}

void test(double first, double step, uint64_t count) {
  double res = lifted_arith_seq_sum(first, step, count);
  double ans = sum(first, step, count);
  printf("sum(%lf, %lf, %lu) = %lf; correct = %lf\n", first, step, count, res,
         ans);
  if (fabs(res - ans) > 1e-5)
    printf("FAILED\n");
}

int main() {
  test(0.0, 1.0, 30);
  test(0.0, 1.1, 5);
  test(1.4, 2.34, 10);
  test(13.3, 1.001, 100);
  return 0;
}
