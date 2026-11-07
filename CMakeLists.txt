cmake_minimum_required (VERSION 3.24)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project ("mk" LANGUAGES C CXX)

# All dependencies in C:/Cpp_Libs are built with the static CRT (/MT).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (MSVC)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:/Zc:preprocessor>
        $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
        $<$<COMPILE_LANGUAGE:C,CXX>:/DNOMINMAX>
        $<$<COMPILE_LANGUAGE:C,CXX>:/D_USE_MATH_DEFINES>
        $<$<COMPILE_LANGUAGE:C,CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:C,CXX>:/bigobj>
    )
else()
    if (WIN32)
        add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
    add_compile_options(-Wall -Wextra -Werror=return-type)
endif()

# Installed test dependency.
set(GTest_DIR "C:/Cpp_Libs/gtest-mt/lib/cmake/GTest" CACHE PATH "")
list(APPEND CMAKE_PREFIX_PATH "C:/Cpp_Libs/gtest-mt")
find_package(GTest CONFIG REQUIRED)

# Installed dependencies (see C:/Cpp_Libs).
set(Boost_ROOT "C:/Cpp_Libs/Boost" CACHE PATH "")
set(Boost_DIR "C:/Cpp_Libs/Boost/lib/cmake/Boost-1.91.0" CACHE PATH "")
set(Boost_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(Boost_USE_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(Boost_USE_DEBUG_LIBS OFF CACHE BOOL "" FORCE)
set(llama_DIR "C:/Cpp_Libs/llamacpp/lib/cmake/llama" CACHE PATH "")
list(APPEND CMAKE_PREFIX_PATH "C:/Cpp_Libs/Boost" "C:/Cpp_Libs/llamacpp")

find_package(llama CONFIG REQUIRED)

add_subdirectory ("mk")
add_subdirectory ("path")
add_subdirectory ("translate")
add_subdirectory ("command_list")
add_subdirectory ("mk_command")
add_subdirectory ("designmode")
add_subdirectory ("test")
