if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD) # 或者 'NEW' 根据你的具体情况选择
endif()
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(ThirdParty STATIC ${srcs})

find_package(ctranslate2 REQUIRED)
target_link_libraries(ThirdParty PUBLIC CTranslate2::ctranslate2)

find_package(cryptopp REQUIRED)
target_link_libraries(ThirdParty PUBLIC cryptopp::cryptopp)


find_package(OpenCV REQUIRED)
target_link_libraries(ThirdParty PUBLIC ${OpenCV_LIBS})


target_link_libraries(ThirdParty PUBLIC sentencepiece)
# Copyright 2019, 2023 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://boost.org/LICENSE_1_0.txt)

# This CMake configuration file provides support for find_package(Boost).
if (MSVC)
   find_package(Boost REQUIRED COMPONENTS  locale filesystem)
else()
	 find_package(Boost  REQUIRED COMPONENTS  locale filesystem )
endif()

target_link_libraries(ThirdParty PUBLIC Boost::locale Boost::filesystem Boost::boost )
#target_link_libraries(ThirdParty PUBLIC Boost::algorithm Boost::filesystem Boost::preprocessor Boost::locale Boost::process Boost::stacktrace)

target_include_directories(ThirdParty PUBLIC include )



