#pragma once

double calc2(float prod, double sum, int x) { return prod + sum - (double)x; }

double calc1(float a, float b, int x) {
  return calc2(a * b, (a + b), x + a / b);
}

double top_small(double a, double b, int x) { return calc1(a, b, x); }
