# RUN: %config-gen-path/config-gen.rb --march rv64im \
# RUN:   --template-dir %config-gen-path/templates -o %t.yaml
# RUN: riscv64-unknown-linux-gnu-clang %s -O0 -o %t.out -march=rv64im -nostdlib -c
# RUN: %bin/mctomir %t.out -o %t.mir -symtab-file=%t.symtab.yaml
# RUN: %bin/llvm-bleach %t.mir --instructions %t.yaml --noinline-instr \
# RUN:   --state-struct-file %t.state.h --symtab-file=%t.symtab.yaml > %t.ll
# RUN: FileCheck %s --input-file=%t.ll

.text
.globl count_to_5
.globl main
.type count_to_5, @function

count_to_5:
  addi a0, a0, 1
  li t2, 5
  beq a0, t2, .Lexit
  # hardcode count_to_5 addr
  li t1, 0
  jr t1
.Lexit:
  ret
.size count_to_5, .-count_to_5  # Force size calculation

main:
  li a0, 0
  jal count_to_5
  addi a0, a0, -5
  ret

# CHECK: define i64 @count_to_5
# CHECK-NEXT: call void @bleach_symtab_add(i64 0, ptr @count_to_5)
# CHECK: [[FUNCADDR:%[0-9]+]] = call ptr @bleach_symtab_lookup
# CHECK: musttail call i64 [[FUNCADDR]]

