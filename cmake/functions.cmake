function(suppress_warnings TARGET_NAME)
  get_target_property(target_options ${TARGET_NAME} COMPILE_OPTIONS)

  if(target_options MATCHES "target_options-NOTFOUND")
    set(target_options "")
  endif()

  if(NOT MSVC)
    # [TODO]: Come up with a better way
  else()
    list(APPEND target_options -w)
  endif()

  set_property(TARGET ${TARGET_NAME} PROPERTY COMPILE_OPTIONS ${target_options})
endfunction()

function(enable_warnings TARGET_NAME)
  if(MSVC)
    target_compile_options(${TARGET_NAME} /W4)
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(
      ${TARGET_NAME}
      PRIVATE -Wall
              -Wextra
              -Wshadow
              -Wcast-align
              -Wunused
              -Wpedantic
              -Wmisleading-indentation
              -Wnull-dereference
              -Wformat=2
              -Wimplicit-fallthrough
              -Wno-missing-field-initializers
              -Wno-nullability-extension)
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_compile_options(
      ${TARGET_NAME}
      PRIVATE -Wall
              -Wextra
              -pedantic
              -Wshadow
              -Wcast-align
              -Wunused
              -Wmisleading-indentation
              -Wduplicated-cond
              -Wduplicated-branches
              -Wlogical-op
              -Wnull-dereference
              -Wformat=2
              -Wimplicit-fallthrough
              -Wno-missing-field-initializers)
  endif()

endfunction(enable_warnings)

option(LINT OFF)
if(${LINT})
  find_program(CLANG_TIDY_EXECUTABLE "clang-tidy")
  set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXECUTABLE}")

  function(target_enable_linter TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY
                                                    "${CLANG_TIDY_COMMAND}")
  endfunction()
else()
  function(target_enable_linter TARGET_NAME)
    # Do nothing
  endfunction()
endif()
