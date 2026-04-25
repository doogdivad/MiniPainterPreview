cmake_minimum_required(VERSION 3.22.1)
project(mini_painter_jni)

set(MINIPAINTER_CORE_DIR "" CACHE PATH "Path to mini_painter_core root")

add_library(mini_painter_jni SHARED mini_painter_jni.cpp)

set(MINIPAINTER_CORE_TARGET "")
if(MINIPAINTER_CORE_DIR AND EXISTS "${MINIPAINTER_CORE_DIR}/CMakeLists.txt")
    add_subdirectory("${MINIPAINTER_CORE_DIR}" mini_painter_core_build)
    foreach(candidate IN ITEMS mini_painter_core minipainter_core mini_painter_core_static)
        if(TARGET ${candidate})
            set(MINIPAINTER_CORE_TARGET ${candidate})
            break()
        endif()
    endforeach()
endif()

if(MINIPAINTER_CORE_TARGET)
    message(STATUS "mini_painter_core detected: ${MINIPAINTER_CORE_TARGET}")
    target_link_libraries(mini_painter_jni PRIVATE ${MINIPAINTER_CORE_TARGET})
    target_compile_definitions(mini_painter_jni PRIVATE MINIPAINTER_CORE_AVAILABLE=1)
else()
    message(STATUS "mini_painter_core not found; building JNI stub mode")
endif()

find_library(log-lib log)
target_link_libraries(mini_painter_jni PRIVATE ${log-lib})
