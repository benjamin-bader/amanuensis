macro(add_test_case SUBJECT TESTNAME)
    set(_TEST_EXE "${SUBJECT}_${TESTNAME}")
    add_executable(${_TEST_EXE} ${ARGN})
    target_link_libraries(${_TEST_EXE} ${SUBJECT} Qt6::Test)
    set_target_properties(${_TEST_EXE} PROPERTIES
        CMAKE_INCLUDE_CURRENT_DIR ON
        FOLDER tests
    )

    if(WIN32)
        target_compile_definitions(${_TEST_EXE} PRIVATE -D_WIN32_WINNT=${MIN_WINNT_VER})
    endif(WIN32)

    add_test(NAME ${TESTNAME} COMMAND ${_TEST_EXE})
endmacro()
