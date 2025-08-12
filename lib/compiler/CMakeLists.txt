find_package(Clang REQUIRED CONFIG)

bleach_add_llvm_based_lib(
  bleach-compiler
  LLVM_COMPONENTS
  support
  core
  SOURCES
  compiler.cpp)

target_sources(
  bleach-compiler
  PUBLIC FILE_SET
         public_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${LLVM_BLEACH_INCLUDE_DIR}
         FILES
         ${LLVM_BLEACH_INCLUDE_DIR}/bleach/compiler/compiler.hpp)
       target_link_libraries(bleach-compiler PRIVATE clangBasic clangDriver clangFrontend clangSupport clangLex clangCodeGen)
target_include_directories(
  bleach-compiler PUBLIC $<BUILD_INTERFACE:${LLVM_BLEACH_INCLUDE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS bleach-compiler FILE_SET public_headers)
