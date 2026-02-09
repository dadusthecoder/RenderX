cmake_minimum_required(VERSION 3.10)
project(HelloTriangle VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#  Create executable
add_executable(HelloTriangle HelloTriangle.cpp Files.cpp)
target_compile_options(HelloTriangle PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>) 
target_include_directories(HelloTriangle PRIVATE "${CMAKE_SOURCE_DIR}/Include")      # RenderX public headers   
#  Link with RenderX library built in /build/bin/<config>/
target_link_directories(HelloTriangle PRIVATE "${CMAKE_SOURCE_DIR}/../../build/bin/${CMAKE_BUILD_TYPE}")
target_link_libraries(HelloTriangle PRIVATE RenderX)
target_link_libraries(HelloTriangle PRIVATE glfw )
