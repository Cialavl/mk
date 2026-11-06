file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(translate STATIC ${srcs})

target_include_directories(translate PUBLIC include)
target_compile_features(translate PUBLIC cxx_std_23)

find_package(Boost 1.91.0 REQUIRED CONFIG COMPONENTS filesystem json)
target_link_libraries(translate PUBLIC llama Boost::filesystem Boost::json)
