set(TEST_TARGET_NAME _test)
add_executable(${TEST_TARGET_NAME} 
main.cxx
constant.hxx
util.hxx
util.cxx
rating.cxx
)
add_subdirectory(logic)
target_link_libraries(${TEST_TARGET_NAME}
    ${TARGET_NAME_LIB}
    CONAN_PKG::catch2
)
target_include_directories(${TEST_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME ${TEST_TARGET_NAME}
    COMMAND ${TEST_TARGET_NAME} -r junit
) 