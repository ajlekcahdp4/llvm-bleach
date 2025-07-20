message("PANDOC: ${PANDOC_PROGRAM}")

function(generate_docs input type)
  set(single_value_args OUTPUT)
  set(multi_value_args)
  cmake_parse_arguments(ARG "" "${single_value_args}" "${multi_value_args}"
                        ${ARGN})
  set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${input}.md)
  add_custom_command(
    OUTPUT ${ARG_OUTPUT}
    COMMAND ${PANDOC_PROGRAM} -s -t ${type} ${INPUT_FILE} -o ${ARG_OUTPUT}
    DEPENDS ${INPUT_FILE}
    COMMENT "(Re-)building documentation from \"${input}\"")
  set(PANDOC_TARGET_NAME docs-${input}-${type})
  add_custom_target(
    ${PANDOC_TARGET_NAME} ALL
    COMMAND
    DEPENDS ${ARG_OUTPUT})
  install(FILES ${ARG_OUTPUT} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endfunction()

generate_docs(bleach man OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/man1/llvm-bleach.1)
generate_docs(mctomir man OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/man1/mctomir.1)
generate_docs(bleach-config-gen man OUTPUT
              ${CMAKE_CURRENT_BINARY_DIR}/man1/bleach-config-gen.1)
