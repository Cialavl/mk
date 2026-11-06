add_executable (mk "mk.cpp" "main.h")
target_link_libraries( mk PUBLIC path)
target_link_libraries( mk PUBLIC command_list)
target_link_libraries( mk PUBLIC designmode)
if (MK_LLAMA_PREBUILT)
    add_custom_command(TARGET mk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LLAMA_RUNTIME_DLLS} $<TARGET_FILE_DIR:mk>
        COMMENT "Copying llama.cpp runtime")
endif()
