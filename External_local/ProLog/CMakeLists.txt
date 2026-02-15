cmake_minimum_required(VERSION 3.16)
project(ProLog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use your local spdlog directory

set(BUILD_SHARED_LIBS OFF)
set(SPDLOG_BUILD_SHARED OFF)
set(SPDLOG_BUILD_STATIC ON)
set(SPDLOG_ENABLE_PCH ON)

#add_subdirectory(../../External/spdlog)

add_library(ProLog STATIC src/ProLog.cpp)

target_include_directories(ProLog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
)

target_link_libraries(ProLog PRIVATE spdlog)



