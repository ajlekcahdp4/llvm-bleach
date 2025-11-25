// No includes needed - no standard library usage

// All the same test functions as before remain unchanged...
float test_basic_float_ops(float a, float b, float c, float d) {
  float add_result = a + b;
  float sub_result = a - b;
  float mul_result = a * b;
  float div_result = a / b;
  return (add_result - sub_result) * (mul_result / div_result) + c - d;
}

double test_basic_double_ops(double a, double b, double c, double d) {
  double add_result = a + b;
  double sub_result = a - b;
  double mul_result = a * b;
  double div_result = a / b;
  return (add_result - sub_result) * (mul_result / div_result) + c - d;
}

float test_fma_ops(float a, float b, float c, float d, float e) {
  float result1 = a * b + c;
  float result2 = a * b - d;
  float neg_a = -a;
  float result3 = neg_a * b - e;
  float result4 = neg_a * b + c;
  return result1 + result2 + result3 + result4;
}

double test_fma_double_ops(double a, double b, double c, double d, double e) {
  double result1 = a * b + c;
  double result2 = a * b - d;
  double neg_a = -a;
  double result3 = neg_a * b - e;
  double result4 = neg_a * b + c;
  return result1 + result2 + result3 + result4;
}

int test_float_int_conversions(float f1, float f2, int i1, int i2, unsigned u1,
                               unsigned u2) {
  int float_to_int1 = (int)f1;
  int float_to_int2 = (int)f2;
  unsigned float_to_uint1 = (unsigned)(f1 > 0 ? f1 : 0.0);
  unsigned float_to_uint2 = (unsigned)(f2 > 0 ? f2 : 0.0);
  float int_to_float1 = (float)i1;
  float int_to_float2 = (float)i2;
  float uint_to_float1 = (float)u1;
  float uint_to_float2 = (float)u2;

  return (long)float_to_int1/16 + (long)float_to_int2/16 + (long)float_to_uint1/16 +
         (long)float_to_uint2/16 + ((long)int_to_float1)/16 + ((long)int_to_float2)/16 +
         ((long)uint_to_float1)/16 + ((long)uint_to_float2)/16;
}

long test_double_long_conversions(double d1, double d2, long l1, long l2,
                                  unsigned long ul1, unsigned long ul2) {
  long double_to_long1 = (long)d1;
  long double_to_long2 = (long)d2;
  unsigned long double_to_ulong1 = (unsigned long)(d1 > 0 ? d1 : 0.0);
  unsigned long double_to_ulong2 = (unsigned long)(d2 > 0 ? d2 : 0.0);
  double long_to_double1 = (double)l1;
  double long_to_double2 = (double)l2;
  double ulong_to_double1 = (double)ul1;
  double ulong_to_double2 = (double)ul2;

  return double_to_long1 /16 + double_to_long2/16 + (long)double_to_ulong1/16 +
         (long)double_to_ulong2/16 + ((long)long_to_double1)/16 +
         ((long)long_to_double2)/16+ ((long)ulong_to_double1)/16 +
         ((long)ulong_to_double2)/16;
}

double test_float_double_conversions(float f1, float f2, double d1, double d2) {
  float double_to_float1 = (float)d1;
  float double_to_float2 = (float)d2;
  double float_to_double1 = (double)f1;
  double float_to_double2 = (double)f2;
  return double_to_float1 + double_to_float2 + float_to_double1 +
         float_to_double2;
}

int test_float_comparisons(float a, float b, float c, float d, float e) {
  int results = 0;
  if (a == b)
    results |= 1;
  if (a < b)
    results |= 2;
  if (a <= c)
    results |= 4;
  if (b > c)
    results |= 8;
  if (b >= a)
    results |= 16;
  if (d != e)
    results |= 32;
  return results;
}

int test_double_comparisons(double a, double b, double c, double d, double e) {
  int results = 0;
  if (a == b)
    results |= 1;
  if (a < b)
    results |= 2;
  if (a <= c)
    results |= 4;
  if (b > c)
    results |= 8;
  if (b >= a)
    results |= 16;
  if (d != e)
    results |= 32;
  return results;
}

float test_sign_ops(float a, float b, float c, float d) {
  float abs_a = (a < 0.0f) ? -a : a;
  float neg_a = -a;
  float neg_b = -b;
  float same_sign = (b >= 0.0f) ? a : -a;
  float opposite_sign = (b >= 0.0f) ? -a : a;
  float xor_sign = (a < 0.0f) != (b < 0.0f) ? -a : a;
  return abs_a + neg_a + neg_b + same_sign + opposite_sign + xor_sign;
}

double test_sign_double_ops(double a, double b, double c, double d) {
  double abs_a = (a < 0.0) ? -a : a;
  double neg_a = -a;
  double neg_b = -b;
  double same_sign = (b >= 0.0) ? a : -a;
  double opposite_sign = (b >= 0.0) ? -a : a;
  double xor_sign = (a < 0.0) != (b < 0.0) ? -a : a;
  return abs_a + neg_a + neg_b + same_sign + opposite_sign + xor_sign;
}

float test_sqrt_minmax(float a, float b, float c, float d, float e) {
  float x = a + b;
  float half = (a + b) / (a + b + a + b);
  for (int i = 0; i < 3; i++) {
    x = half * (x + a / x);
  }
  float min_ab = a < b ? a : b;
  float max_bc = b > c ? b : c;
  float min_de = d < e ? d : e;
  float max_ea = e > a ? e : a;
  return x + min_ab + max_bc + min_de + max_ea;
}

double test_sqrt_minmax_double(double a, double b, double c, double d,
                               double e) {
  double x = a + b;
  double half = (a + b) / (a + b + a + b);
  for (int i = 0; i < 3; i++) {
    x = half * (x + a / x);
  }
  double min_ab = a < b ? a : b;
  double max_bc = b > c ? b : c;
  double min_de = d < e ? d : e;
  double max_ea = e > a ? e : a;
  return x + min_ab + max_bc + min_de + max_ea;
}

int test_classify_ops(float f1, float f2, float f3, double d1, double d2,
                      double d3) {
  int result = 0;
  if (f1 != f1)
    result |= 1;
  float large_float = f1 * f1 + f2 * f2 + f3 * f3;
  if (f1 > large_float && f1 == f1)
    result |= 2;
  if (f1 < -large_float && f1 == f1)
    result |= 4;
  if (f1 == 0.0f)
    result |= 8;
  if (f1 > 0.0f && f1 == f1)
    result |= 16;
  if (f1 < 0.0f && f1 == f1)
    result |= 32;

  if (d1 != d1)
    result |= 64;
  double large_double = d1 * d1 + d2 * d2 + d3 * d3;
  if (d1 > large_double && d1 == d1)
    result |= 128;
  if (d1 < -large_double && d1 == d1)
    result |= 256;
  if (d1 == 0.0)
    result |= 512;
  if (d1 > 0.0 && d1 == d1)
    result |= 1024;
  if (d1 < 0.0 && d1 == d1)
    result |= 2048;
  return result;
}

float test_complex_float_expr(float a, float b, float c, float d, float e,
                              float f, int one_int) {
  float one = (float)one_int;
  float temp1 = (a + b) * (c - d);
  float temp2 = (e * f) / (a + one);
  float temp3 = test_sqrt_minmax(temp1, temp2, a, b, c);
  float temp4 = (temp1 < temp2) ? temp1 : temp2;
  float temp5 = (temp3 > temp4) ? temp3 : temp4;
  return temp1 + temp2 - temp3 * temp4 / temp5;
}

double test_complex_double_expr(double a, double b, double c, double d,
                                double e, double f, int one_int) {
  double one = (double)one_int;
  double temp1 = (a + b) * (c - d);
  double temp2 = (e * f) / (a + one);
  double temp3 = test_sqrt_minmax_double(temp1, temp2, a, b, c);
  double temp4 = (temp1 < temp2) ? temp1 : temp2;
  double temp5 = (temp3 > temp4) ? temp3 : temp4;
  return temp1 + temp2 - temp3 * temp4 / temp5;
}

// NEW TOP-LEVEL FUNCTION: Mix of GPR and FPR arguments
__attribute__((noinline)) double
top_function_mixed(int i1, int i2, unsigned u1, unsigned u2, long l1, long l2,
                   unsigned long ul1, unsigned long ul2, float f1, float f2,
                   float f3, float f4, double d1, double d2, double d3,
                   double d4) {

  double result = 0.0;
  result += test_float_int_conversions(f1, f2, i1, i2, u1, u2);
  return result;

  // Test basic operations
  result += test_basic_float_ops(f1, f2, f3, f4);
  result += test_basic_double_ops(d1, d2, d3, d4);

  // Test FMA operations
  result += test_fma_ops(f1, f2, f3, f4, f1);        // Reuse f1 for 5th param
  result += test_fma_double_ops(d1, d2, d3, d4, d1); // Reuse d1 for 5th param

  // Test conversions
    result += test_double_long_conversions(d1, d2, l1, l2, ul1, ul2);
  result += test_float_double_conversions(f3, f4, d3, d4);

  // Test comparisons
  result +=
      test_float_comparisons(f1, f2, f3, f4, f1); // Reuse f1 for 5th param
  result +=
      test_double_comparisons(d1, d2, d3, d4, d1); // Reuse d1 for 5th param

  // Test sign operations
  result += test_sign_ops(f1, f2, f3, f4);
  result += test_sign_double_ops(d1, d2, d3, d4);

  // Test sqrt and min/max
  result += test_sqrt_minmax(f1, f2, f3, f4, f1); // Reuse f1 for 5th param
  result +=
      test_sqrt_minmax_double(d1, d2, d3, d4, d1); // Reuse d1 for 5th param

  // Test classification
  result += test_classify_ops(f1, f2, f3, d1, d2, d3);

  // Test complex expressions (use i1 as "1" value)
  int one_val = (i1 == 0) ? 1 : i1; // Ensure we have at least 1
  result +=
      test_complex_float_expr(f1, f2, f3, f4, f1, f2, one_val); // Reuse params
  result +=
      test_complex_double_expr(d1, d2, d3, d4, d1, d2, one_val); // Reuse params

  return result;
}

// Entry point with mixed GPR and FPR arguments
double test_all_operations(int i1, int i2, unsigned u1, unsigned u2, long l1,
                           long l2, unsigned long ul1, unsigned long ul2,
                           float f1, float f2, float f3, float f4, double d1,
                           double d2, double d3, double d4) {
  return top_function_mixed(i1, i2, u1, u2, l1, l2, ul1, ul2, f1, f2, f3, f4,
                            d1, d2, d3, d4) *
         f3;
}
