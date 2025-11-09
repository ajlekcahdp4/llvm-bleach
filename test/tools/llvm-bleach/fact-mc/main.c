__attribute__((noinline)) unsigned long long fact(unsigned long long num) {
  unsigned long long res = 1;
  for (unsigned long long i = 2; i <= num; ++i)
    res *= i;
  return res;
}

int main() { return fact(5); }
