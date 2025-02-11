long long fact(long long num) {
  long long res = 1;
  for (long long i = 2; i <= num; ++i)
    res *= i;
  return res;
}
