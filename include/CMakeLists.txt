# Special common target for headers that propagates

# needed include directories and dependencies.

add_library(bleach-headers INTERFACE)

target_include_directories(bleach-headers
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bleach/version.inc.in
               ${CMAKE_CURRENT_BINARY_DIR}/bleach/version.inc)
