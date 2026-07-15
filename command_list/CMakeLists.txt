file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(command_list STATIC ${srcs})

target_include_directories(command_list PUBLIC include)
target_link_libraries( command_list PUBLIC path)

add_definitions(-DBOOST_TIMER_ENABLE_DEPRECATED)
find_package(Boost 1.91.0 REQUIRED CONFIG COMPONENTS
    atomic
    chrono
    container
    context
    coroutine
    date_time
    exception
    filesystem
    graph
    iostreams
    json
    locale
    log
    log_setup
    nowide
    program_options
    random
    regex
    serialization
    system
    thread
    timer
    type_erasure
    unit_test_framework
    url
    wave
    wserialization
)
target_link_libraries(command_list PUBLIC Boost::boost Boost::locale Boost::filesystem)
target_link_libraries(command_list PUBLIC sentencepiece)
find_package(ctranslate2 REQUIRED CONFIG)
target_link_libraries(command_list PUBLIC CTranslate2::ctranslate2)


