#include <STATE>

#include <stdio.h>

struct register_state s = {};
int main() {
  bleached_main(&s);
  printf("Hello\n");
  printf("result: %ld\n", s.GPR[10]);
  return 0;
}
