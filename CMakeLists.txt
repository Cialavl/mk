cmake_minimum_required (VERSION 3.10)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)  # 让Boost_ROOT等变量生效
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project ("mk")

if (MSVC)
    add_compile_options(/Zc:preprocessor /utf-8 /DNOMINMAX /D_USE_MATH_DEFINES /EHsc /bigobj)
else()
    if (WIN32)
        add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
    add_compile_options(-Wall -Wextra -Werror=return-type)
endif()
include(FetchContent)

FetchContent_Declare(
  sentencepiece
  GIT_REPOSITORY https://github.com/google/sentencepiece.git
  GIT_TAG        v0.2.1   # 建议固定版本
)

FetchContent_MakeAvailable(sentencepiece)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)  # 必须放在 MakeAvailable 之前
FetchContent_MakeAvailable(googletest)
# 包含子项目。
set(Boost_ROOT "C:/Cpp_Libs/Boost")
add_subdirectory ("mk")
add_subdirectory ("path")
add_subdirectory ("command_list")
add_subdirectory ("mk_command")
add_subdirectory ("designmode")
add_subdirectory ("test")
