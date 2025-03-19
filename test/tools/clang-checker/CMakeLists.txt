if(BASH_PROGRAM)
  add_test(NAME test.clang-checker
    COMMAND 
    ${BASH_PROGRAM} ${CLANG_CHECKER_DIR}/clang-checker.sh  
  )
endif()
