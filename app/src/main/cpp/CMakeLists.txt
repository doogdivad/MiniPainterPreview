cmake_minimum_required(VERSION 3.22.1)
project(mini_painter_jni)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mini_painter_jni SHARED mini_painter_jni.cpp)

find_library(log-lib log)
target_link_libraries(mini_painter_jni ${log-lib})

set(MINI_PAINTER_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../mini_painter_core")
if(EXISTS "${MINI_PAINTER_CORE_DIR}/CMakeLists.txt")
    add_subdirectory(${MINI_PAINTER_CORE_DIR} mini_painter_core_build)
    target_link_libraries(mini_painter_jni mini_painter_core)
    target_compile_definitions(mini_painter_jni PRIVATE MINI_NATIVE_CORE_AVAILABLE=1)
endif()
