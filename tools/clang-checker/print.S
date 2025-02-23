.global print
.global print_impl

.text
print:
  addi sp,sp,-96
  sd ra, 96(sp) // save ra
  sd t3, 88(sp)
  sd t4, 80(sp)
  sd t5, 72(sp)
  sd t6, 64(sp)
  sd t0, 56(sp)
  sd t1, 48(sp)
  sd t2, 40(sp)
  sd a0, 32(sp)
  sd a1, 24(sp)
  sd a2, 16(sp)
  sd a3, 8(sp)
  call print_impl
  ld ra, 96(sp)
  ld t3, 88(sp)
  ld t4, 80(sp)
  ld t5, 72(sp)
  ld t6, 64(sp)
  ld t0, 56(sp)
  ld t1, 48(sp)
  ld t2, 40(sp)
  ld a0, 32(sp)
  ld a1, 24(sp)
  ld a2, 16(sp)
  ld a3, 8(sp)
  addi sp,sp,96
  ret
