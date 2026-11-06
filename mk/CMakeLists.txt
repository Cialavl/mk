add_executable (mk "mk.cpp" "main.h")
target_link_libraries( mk PUBLIC path)
target_link_libraries( mk PUBLIC command_list)
target_link_libraries( mk PUBLIC designmode)
