cmake_minimum_required(VERSION 3.20)

project(MiniPainterPreview VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINI_PAINTER_BUILD_TESTS "Build tests" ON)

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

add_executable(mini_painter_cli
    src/cli/main.cpp
)

target_link_libraries(mini_painter_cli PRIVATE mini_painter_core)

if(MINI_PAINTER_BUILD_TESTS)
    enable_testing()
    add_executable(mini_painter_tests tests/test_project.cpp)
    target_link_libraries(mini_painter_tests PRIVATE mini_painter_core)
    add_test(NAME mini_painter_tests COMMAND mini_painter_tests)
endif()
