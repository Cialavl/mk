file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)

enable_testing()
add_executable (test ${srcs})
target_link_directories(test PUBLIC include)

target_link_libraries( test PUBLIC GTest::gtest GTest::gtest_main)
target_link_libraries( test PUBLIC path)
target_link_libraries( test PUBLIC command_list)
target_link_libraries( test PUBLIC designmode)

add_custom_command(
    TARGET test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/resources/test.jpg
    $<TARGET_FILE_DIR:test>/test.jpg
    COMMENT "Copying test.jpg to output directory"
)
if (MK_LLAMA_PREBUILT)
    add_custom_command(TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LLAMA_RUNTIME_DLLS} $<TARGET_FILE_DIR:test>
        COMMENT "Copying llama.cpp runtime")
endif()
