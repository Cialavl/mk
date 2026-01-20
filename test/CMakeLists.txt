file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)


enable_testing()
add_executable (test ${srcs})
target_link_directories(test PUBLIC include)

# 获取可执行文件的输出目录
get_target_property(EXE_OUTPUT_DIR test RUNTIME_OUTPUT_DIRECTORY)
if(NOT EXE_OUTPUT_DIR)
    set(EXE_OUTPUT_DIR $<TARGET_FILE_DIR:test>)
endif()



# 定义复制资源文件夹的命令
add_custom_command(
    TARGET test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXE_OUTPUT_DIR}/model
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/model ${EXE_OUTPUT_DIR}/model

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/resources/test.jpg
    ${EXE_OUTPUT_DIR}/test.jpg
    COMMENT "Copying test.jpg to output directory Copying resources/model to output directory"
)


target_link_libraries( test PUBLIC GTest::gtest GTest::gtest_main)
target_link_libraries( test PUBLIC path)
target_link_libraries( test PUBLIC command_list)
target_link_libraries( test PUBLIC designmode)

