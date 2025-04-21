bleach_add_llvm_based_lib(mctomir-riscv-target LLVM_COMPONENTS target SOURCES
                          riscv-target.cpp)

target_include_directories(mctomir-riscv-target
                           PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
