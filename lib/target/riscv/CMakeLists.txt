bleach_add_llvm_based_lib(
  bleach-riscv-target
  LLVM_COMPONENTS
  target
  SOURCES
  riscv-target.cpp
)

target_include_directories(bleach-riscv-target PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
