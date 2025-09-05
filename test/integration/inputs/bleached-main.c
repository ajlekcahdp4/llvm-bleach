#include <STATE>

#include <stdio.h>

extern int64_t bleached_main(struct register_state *s);

int main() {
  struct register_state s = {};
  bleached_main(&s);
  printf("result: %ld\n", s.GPR[10]);
  return 0;
}
