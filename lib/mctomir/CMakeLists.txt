bleach_add_llvm_based_lib(
  mctomir-lifter
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  SOURCES
  elf-loader.cpp
  mctomir-transform.cpp)

target_include_directories(
  mctomir-lifter PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
