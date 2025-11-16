#pragma once

double calc2(double prod, double sum, int x) { return prod + sum - (double)x; }

double calc1(double a, double b, int x) { return calc2(a * b, (a + b), x); }

double top_small(double a, double b, int x) { return calc1(a, b, x); }
