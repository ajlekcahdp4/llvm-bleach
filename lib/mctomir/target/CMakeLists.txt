add_subdirectory(riscv)

bleach_add_llvm_based_lib(mctomir-target-selector LLVM_COMPONENTS target SOURCES
                          target-selector.cpp)

target_include_directories(mctomir-target-selector
                           PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(mctomir-target-selector PUBLIC mctomir-riscv-target)
