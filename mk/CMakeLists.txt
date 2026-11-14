add_executable (mk "mk.cpp" "main.h" "serve.cpp" "serve.h")
target_link_libraries( mk PUBLIC path)
target_link_libraries( mk PUBLIC command_list)
target_link_libraries( mk PUBLIC designmode)
target_link_libraries( mk PUBLIC assist)
target_link_libraries( mk PRIVATE winmm)

if (MSVC)
    # openblas.lib is built with Intel Fortran and requests this runtime, which is not installed.
    target_link_options(mk PRIVATE /NODEFAULTLIB:flang_rt.runtime.dynamic.lib)
    target_link_options(mk PRIVATE /NODEFAULTLIB:flang_rt.runtime.lib)
endif()

# PaddleOCR resolves its pipeline yaml from a build-time path; ship one next to mk.exe.
add_custom_command(TARGET mk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mk>/configs"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assist/configs/OCR.yaml"
        "$<TARGET_FILE_DIR:mk>/configs/OCR.yaml"
    COMMENT "Copying PaddleOCR pipeline config")
