#include <state.h>

#include <stdio.h>

extern int bleached_main(struct register_state *s);

int main() {
  struct register_state s = {};
  bleached_main(&s);
  printf("x10: %lx, x11: %lx\n", s.GPR[10], s.GPR[11]);
  printf("result: %ld", s.GPR[10]);
  return 0;
}
