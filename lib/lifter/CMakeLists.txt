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

target_link_libraries(bleach-lifter PUBLIC yaml-cpp::yaml-cpp)
target_sources(
  bleach-lifter
  PUBLIC FILE_SET
         public_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${LLVM_BLEACH_INCLUDE_DIR}
         FILES
         ${LLVM_BLEACH_INCLUDE_DIR}/bleach/lifter/instr-impl.hpp
         ${LLVM_BLEACH_INCLUDE_DIR}/bleach/lifter/lifter.hpp)
target_include_directories(
  bleach-lifter PUBLIC $<BUILD_INTERFACE:${LLVM_BLEACH_INCLUDE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS bleach-lifter FILE_SET public_headers)
