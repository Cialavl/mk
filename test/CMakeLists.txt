file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)

enable_testing()
add_executable (test ${srcs})

target_link_libraries( test PUBLIC GTest::gtest GTest::gtest_main)
target_link_libraries( test PUBLIC path)
target_link_libraries( test PUBLIC command_list)
target_link_libraries( test PUBLIC designmode)
