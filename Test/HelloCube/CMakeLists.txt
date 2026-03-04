cmake_minimum_required(VERSION 3.10)
project(HelloCube VERSION 1.0)

find_package(glfw3 CONFIG REQUIRED)

add_executable(HelloCube HelloCube.cpp Files.cpp)

target_compile_options(HelloCube PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_include_directories(HelloCube PRIVATE "${CMAKE_SOURCE_DIR}/Include")
target_link_libraries(HelloCube PRIVATE RenderX)
target_link_libraries(HelloCube PRIVATE glfw)
