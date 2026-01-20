file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(command_list STATIC ${srcs})

target_include_directories(command_list PUBLIC include)
target_link_libraries( command_list PUBLIC path)
find_package(Boost REQUIRED COMPONENTS locale)
target_link_libraries(command_list PUBLIC Boost::boost Boost::locale)
target_link_libraries(command_list PUBLIC sentencepiece)
find_package(ctranslate2 REQUIRED)
target_link_libraries(command_list PUBLIC CTranslate2::ctranslate2)


