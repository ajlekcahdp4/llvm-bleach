add_subdirectory(riscv)

bleach_add_llvm_based_lib(bleach-target-selector LLVM_COMPONENTS target SOURCES
                          target-selector.cpp)

target_include_directories(bleach-target-selector
                           PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(bleach-target-selector PUBLIC bleach-riscv-target)
