find_program(lit NAMES llvm-lit llvm-lit.py lit)

if(lit-NOTFOUND)
  message("lit tool not found. Skipping testing")
  return()
endif()

configure_file(lit.site.cfg.py.in lit.site.cfg.py)
add_test(NAME mctomir-tool-test COMMAND ${lit} --verbose
                                            ${CMAKE_CURRENT_BINARY_DIR})
set_property(
  TEST mctomir-tool-test
  PROPERTY ENVIRONMENT PATH="${CMAKE_BINARY_DIR}/bin:$ENV{PATH}"
  PROPERTY ENVIRONMENT RUN_PATH=${CMAKE_CURRENT_BINARY_DIR})
