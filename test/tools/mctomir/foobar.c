// COM: -O2 for compiler to inline call to foo
// RUN: gcc -c %s -o %t.o -O0
// RUN: %bin/mctomir %t.o -o - | FileCheck %s -check-prefix=O0
// RUN: gcc -c %s -o %t.o -O2
// RUN: %bin/mctomir %t.o -o - | FileCheck %s -check-prefix=O2

int foo(int x) { return 42 * x; }

int bar(int x) { return foo(x) * 42; }

// O0: name: foo
// O0: body:
// O0-NEXT: bb.0:
// O0: $eax = IMUL32{{.*}} $eax, 42
// O0: RET64

// O0: name: bar
// O0: body:
// O0-NEXT: bb.0:
// O0: CALL
// O0: $eax = IMUL32{{.*}} $eax, 42
// O0: RET64

// O2: name: foo
// O2: body:
// O2-NEXT: bb.0:
// O2-NEXT: $eax = IMUL32{{.*}} $edi, 42
// O2-NEXT: RET64

// O2: name: bar
// O2: body:
// O2-NEXT: bb.0:
// O2-NEXT: $eax = IMUL32{{.*}} $edi, 1764
// O2-NEXT: RET64
