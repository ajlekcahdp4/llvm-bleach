#include <STATE>

#include <stdio.h>

int main() {
  struct register_state s = {};
  bleached_main(&s);
  printf("Hello\n");
  printf("result: %ld\n", s.GPR[10]);
  return 0;
}
