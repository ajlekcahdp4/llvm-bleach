bleach_add_llvm_based_lib(
  bleach-passes
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  SOURCES
  block-ir-builder.cpp
  redundant-branch-eraser.cpp)

target_include_directories(bleach-passes PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(bleach-passes PUBLIC bleach-lifter)
