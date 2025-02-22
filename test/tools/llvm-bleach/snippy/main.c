#include <stdio.h>

typedef struct {
  long long regs[32];
  long long stack[1000];
} state;

extern void SnippyFunction(state *st);

static state regs = {};

static void lifted_snippy() { SnippyFunction(&regs); }

void print(state *st) {
  printf("%lld %lld %lld %lld\n", st->regs[10], st->regs[11], st->regs[12],
         st->regs[13]);
}

int main() {
  lifted_snippy();
  return 0;
}
