find_program(CLANG_COMPILER clang HINTS ${BLEACH_CLANG_COMPILER_PATH})
if (NOT CLANG_COMPILER)
  message(FATAL_ERROR "No clang compiler found for building symtab.cpp. Try specifying BLEACH_CLANG_COMPILER_PATH")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/symtab-ir.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.1
  COPYONLY
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h
  COMMAND
    ${CLANG_COMPILER}
    "-xc++" "-c" "-S" "-emit-llvm"
    "-o" "${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.2"
    "${CMAKE_CURRENT_SOURCE_DIR}/symtab.cpp"
  COMMAND
    ${CMAKE_COMMAND} -E "echo" "\")\\\";}}\"" > ${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.3
  COMMAND
    ${CMAKE_COMMAND} -E "cat"
    "${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.1"
    "${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.2"
    "${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h.3" >
    "${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h"
  COMMAND
    ${CMAKE_COMMAND}
    -DINPUT_FILE_PATH="${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h"
    -DOUTPUT_FILE_PATH="${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/replace_ir_target.cmake
  COMMENT "Pre-compiling symtab.cpp"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/symtab.cpp" ${CMAKE_CURRENT_SOURCE_DIR}/symtab-ir.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/replace_ir_target.cmake
)

add_custom_target(load_file_contents ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/symtab-ir.h
)


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

add_dependencies(bleach-lifter load_file_contents)

target_link_libraries(bleach-lifter PUBLIC yaml-cpp::yaml-cpp mctomir-lifter)
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
  bleach-lifter PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                       $<BUILD_INTERFACE:${LLVM_BLEACH_INCLUDE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS bleach-lifter FILE_SET public_headers)
