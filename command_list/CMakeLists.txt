file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(command_list STATIC ${srcs})

target_include_directories(command_list PUBLIC include)
target_link_libraries(command_list PUBLIC path translate)

add_definitions(-DBOOST_TIMER_ENABLE_DEPRECATED)
find_package(Boost 1.91.0 REQUIRED CONFIG COMPONENTS filesystem locale thread container atomic chrono regex)
target_link_libraries(command_list PUBLIC Boost::boost Boost::locale Boost::filesystem)
