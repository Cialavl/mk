
add_executable (mk "mk.cpp" "main.h")
target_link_libraries( mk PUBLIC path)
target_link_libraries( mk PUBLIC command_list)
target_link_libraries( mk PUBLIC designmode)

get_target_property(EXE_OUTPUT_DIR mk RUNTIME_OUTPUT_DIRECTORY)
if(NOT EXE_OUTPUT_DIR)
    set(EXE_OUTPUT_DIR $<TARGET_FILE_DIR:mk>)
endif()

add_custom_command(
    TARGET mk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXE_OUTPUT_DIR}/model
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/model ${EXE_OUTPUT_DIR}/model
    COMMENT "Copying resources/model to mk output directory"
)


