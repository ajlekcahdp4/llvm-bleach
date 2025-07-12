bleach_add_llvm_based_lib(
  bleach-passes
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  analysis
  SOURCES
  block-ir-builder.cpp
  redundant-branch-eraser.cpp)

target_link_libraries(bleach-passes PUBLIC bleach-lifter)
target_sources(
  bleach-passes
  PUBLIC FILE_SET
         public_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${LLVM_BLEACH_INCLUDE_DIR}
         FILES
         ${LLVM_BLEACH_INCLUDE_DIR}/bleach/passes/redundant-branch-eraser.hpp
         ${LLVM_BLEACH_INCLUDE_DIR}/bleach/passes/block-ir-builder.hpp)
target_include_directories(
  bleach-passes PUBLIC $<BUILD_INTERFACE:${LLVM_BLEACH_INCLUDE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS bleach-passes FILE_SET public_headers)
