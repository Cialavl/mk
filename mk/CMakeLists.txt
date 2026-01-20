
add_executable (mk "mk.cpp" "main.h")
target_link_libraries( mk PUBLIC path)
target_link_libraries( mk PUBLIC command_list)
target_link_libraries( mk PUBLIC designmode)



# TODO: 如有需要，请添加测试并安装目标。


