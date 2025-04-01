find_package(yaml-cpp REQUIRED CONFIG)

bleach_add_llvm_based_lib(
  bleach-lifter
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  SOURCES
  instr-impl.cpp
  lifter.cpp)

target_include_directories(bleach-lifter PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(bleach-lifter PUBLIC yaml-cpp::yaml-cpp)
