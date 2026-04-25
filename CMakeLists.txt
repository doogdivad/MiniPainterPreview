cmake_minimum_required(VERSION 3.20)

project(MiniPainterPreview VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINI_PAINTER_BUILD_TESTS "Build tests" ON)
find_package(OpenCV QUIET COMPONENTS core imgcodecs imgproc)

add_library(mini_painter_core STATIC
    src/c_api.cpp
    src/project_store.cpp
)

target_include_directories(mini_painter_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(mini_painter_core PUBLIC cxx_std_17)
if(OpenCV_FOUND)
    target_compile_definitions(mini_painter_core PRIVATE MINI_PAINTER_HAS_OPENCV=1)
    target_link_libraries(mini_painter_core PRIVATE ${OpenCV_LIBS})
else()
    message(WARNING "OpenCV was not found. mini_analyse_image_quality will return MINI_ERROR_NOT_IMPLEMENTED.")
endif()

add_executable(mini_painter_cli
    src/cli/main.cpp
)

target_link_libraries(mini_painter_cli PRIVATE mini_painter_core)

if(MINI_PAINTER_BUILD_TESTS)
    enable_testing()
    add_executable(mini_painter_tests tests/test_project.cpp)
    if(OpenCV_FOUND)
        target_compile_definitions(mini_painter_tests PRIVATE MINI_PAINTER_HAS_OPENCV=1)
        target_link_libraries(mini_painter_tests PRIVATE mini_painter_core ${OpenCV_LIBS})
    else()
        target_link_libraries(mini_painter_tests PRIVATE mini_painter_core)
    endif()
    add_test(NAME mini_painter_tests COMMAND mini_painter_tests)
endif()
