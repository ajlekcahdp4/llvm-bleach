// RUN: clang %s -O0 -nostdlib -o %t.out
// RUN: %bin/mctomir %t.out -o - -symtab-file=%t.yaml
// RUN: bash %S/inputs/extract-value-and-size.sh %t.out > %t.symbols.json
// Check fact start addr
// RUN: cat %t.symbols.json | jq '.fact.Value' > %t-fact-start.ref
// RUN: cat %t.symbols.json | jq '.fact.Size' > %t-fact-size.ref
// RUN: cat %t.yaml | yq '.[] | select(.name == "fact").start' > \
// RUN:   %t-fact-start.val
// RUN: cat %t.yaml | yq '.[] | select(.name == "fact").end' > \
// RUN:   %t-fact-end.val
// RUN: diff %t-fact-start.val %t-fact-start.ref
// Check fact end addr
// RUN: bash %S/inputs/sum.sh %t-fact-start.ref %t-fact-size.ref > \
// RUN:   %t-fact-end.ref
// RUN: diff %t-fact-end.val %t-fact-end.ref

// Check main start addr
// RUN: cat %t.symbols.json | jq '.main.Value' > %t-main-start.ref
// RUN: cat %t.symbols.json | jq '.main.Size' > %t-main-size.ref
// RUN: cat %t.yaml | yq '.[] | select(.name == "main").start' > \
// RUN:   %t-main-start.val
// RUN: cat %t.yaml | yq '.[] | select(.name == "main").end' > \
// RUN:   %t-main-end.val
// RUN: diff %t-main-start.val %t-main-start.ref
// Check main end addr
// RUN: bash %S/inputs/sum.sh %t-main-start.ref %t-main-size.ref > \
// RUN:   %t-main-end.ref
// RUN: diff %t-main-end.val %t-main-end.ref

static unsigned fact(unsigned num) {
  unsigned res = 1;
  while (num)
    res *= num--;
  return res;
}

int main() { return fact(5); }

// CHECK: fuck
