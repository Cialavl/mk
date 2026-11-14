set(PADDLE_ROOT "C:/Cpp_Libs/mt/Release" CACHE PATH "PaddleOCR / Paddle Inference root")

# PaddleOCR headers use std::result_of, removed in C++20; isolate them in a C++17 target.
file(GLOB ocr_srcs CONFIGURE_DEPENDS src/ocr_engine.cpp)
add_library(assist_ocr STATIC ${ocr_srcs} vendor/clipper.cpp vendor/flang_stub.c)
set_target_properties(assist_ocr PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(assist_ocr
    PUBLIC include
    PRIVATE
        vendor
        vendor/polyclipping
        "${PADDLE_ROOT}/PaddleOCR/include"
        "${PADDLE_ROOT}/abseil/include"
        "${PADDLE_ROOT}/opencv/include"
        "${PADDLE_ROOT}/paddle_inference/paddle/include"
        "${PADDLE_ROOT}/paddle_inference/third_party/install/yaml-cpp/include"
        "${PADDLE_ROOT}/paddle_inference/third_party/install/glog/include"
        "${PADDLE_ROOT}/paddle_inference/third_party/install/gflags/include")

find_package(Boost 1.91.0 REQUIRED CONFIG COMPONENTS json)
target_link_libraries(assist_ocr PRIVATE Boost::json)

file(GLOB ABSL_LIBS "${PADDLE_ROOT}/abseil/lib/*.lib")
file(GLOB OPENCV_LIBS "${PADDLE_ROOT}/opencv/x64/vc18/staticlib/*.lib")
# libtorch (through tvlib) already provides ittnotify and protobuf; skip OpenCV's copies.
list(FILTER OPENCV_LIBS EXCLUDE REGEX "/ittnotify\\.lib$")
list(FILTER OPENCV_LIBS EXCLUDE REGEX "/libprotobuf\\.lib$")
list(FILTER OPENCV_LIBS EXCLUDE REGEX "/libprotobuf-lite\\.lib$")

target_link_directories(assist_ocr PUBLIC
    "${PADDLE_ROOT}/PaddleOCR/lib"
    "${PADDLE_ROOT}/paddle_inference/paddle/lib"
    "${PADDLE_ROOT}/paddle_inference/third_party/install/yaml-cpp/lib"
    "${PADDLE_ROOT}/paddle_inference/third_party/install/glog/lib"
    "${PADDLE_ROOT}/paddle_inference/third_party/install/gflags/lib"
    "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3/lib/x64")

target_link_libraries(assist_ocr PUBLIC
    ppocr_core.lib
    ${OPENCV_LIBS}
    paddle_inference.lib
    yaml-cpp.lib
    glog.lib
    gflags_static.lib
    ${ABSL_LIBS}
    cudart.lib cuda.lib cublas.lib cufft.lib
    nppc.lib nppial.lib nppicc.lib nppidei.lib nppif.lib nppig.lib nppim.lib nppist.lib nppitc.lib npps.lib
    comctl32.lib gdi32.lib ole32.lib setupapi.lib ws2_32.lib crypt32.lib user32.lib advapi32.lib shell32.lib
    winmm.lib delayimp.lib)

# Loaded on first OCR call; plain translation never needs Paddle on disk.
target_link_options(assist_ocr PUBLIC "/DELAYLOAD:paddle_inference.dll")

# Text to speech wrapper; the rest of the project stays on C++23.
file(GLOB tts_srcs CONFIGURE_DEPENDS src/tts_engine.cpp src/utf8_path.cpp)
add_library(assist STATIC ${tts_srcs})
target_include_directories(assist PUBLIC include)
target_compile_features(assist PUBLIC cxx_std_23)
target_link_libraries(assist PUBLIC assist_ocr)
target_link_libraries(assist PUBLIC winmm)

if (TARGET tvlib)
    target_link_libraries(assist PUBLIC tvlib tvlib_link_options)
endif()
