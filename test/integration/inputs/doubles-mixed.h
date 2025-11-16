#pragma once

double complex_mixed(double a, double b, double c, double d) {
  double part1 = (a + b) * (unsigned)(c - d);
  double part2 = (a * d) + (int)(b * c);
  double part3 = (a / c) * (long long)(b / d);
  return part1 + part2 - part3;
}
