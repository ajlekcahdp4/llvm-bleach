#include <stddef.h>
#include <stdio.h>

extern void SnippyFunction();

void print_impl(long long a, long long b, long long c, long long d) {
  printf("%lld %lld %lld %lld\n", a, b, c, d);
}

int main() {
  SnippyFunction();
  return 0;
}
