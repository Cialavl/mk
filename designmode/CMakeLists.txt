file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(designmode STATIC ${srcs})
target_include_directories(designmode PUBLIC include)
target_link_libraries(designmode PUBLIC mk_command)
# Boost header-only libraries
set(BOOST_ROOT "C:/Cpp_Libs/boost") 
find_package(Boost CONFIG REQUIRED COMPONENTS locale filesystem)
target_link_libraries(designmode PUBLIC Boost::boost Boost::locale Boost::filesystem)