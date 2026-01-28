cmake_minimum_required(VERSION 3.10)

project(RenderX VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")



# ────────────────────────────────────────────────────────────────
#   OUTPUT FOLDERS (Debug/Release separated)
# ────────────────────────────────────────────────────────────────

# Final binaries (.exe .dll .so .dylib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Libraries (.dll .so .lib .a)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Static libs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Object files
set(CMAKE_OBJECT_PATH_MAX 256)
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-int")

# Multi-config generators (VS, Xcode)
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_OBJECT_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin-int/${CONFIG}/${PROJECT_NAME}")
endforeach()

# ────────────────────────────────────────────────────────────────
#   External Libraries
# ────────────────────────────────────────────────────────────────
add_subdirectory(External/glm)
add_subdirectory(External/spdlog)
add_subdirectory(External/glfw)
add_subdirectory(External_local/glew-2.1.0/build/cmake)
add_subdirectory(External_local/ProLog)


add_subdirectory(Test/HelloTriangle)

# ────────────────────────────────────────────────────────────────
#   Source Files
# ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/RenderX/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/RenderX/*.h"
    "${CMAKE_SOURCE_DIR}/src/OpenGL/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/OpenGL/*.h"
    "${CMAKE_SOURCE_DIR}/src/Vulkan/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/Vulkan/*.h"
)

find_package(Vulkan REQUIRED)
# ────────────────────────────────────────────────────────────────
#   RenderX Shared Library
# ────────────────────────────────────────────────────────────────
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERX_BUILD_DLL)

target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_SOURCE_DIR}/src"
    PUBLIC  "${CMAKE_SOURCE_DIR}/Include"
    PUBLIC  "${CMAKE_SOURCE_DIR}/External/glm"
    PUBLIC "${CMAKE_SOURCE_DIR}/External/spdlog/include"
    PUBLIC "${CMAKE_SOURCE_DIR}/External/glfw/include"
    PRIVATE "${CMAKE_SOURCE_DIR}/External_local/glew-2.1.0/include"
    PUBLIC "${CMAKE_SOURCE_DIR}/External_local/ProLog/include"
)


target_link_directories(RenderX PUBLIC External_local/ProLog/bin)

target_link_libraries(${PROJECT_NAME}
    PRIVATE glm
    PRIVATE spdlog
    PRIVATE glfw
    PRIVATE  ProLog
    PUBLIC Vulkan::Vulkan
    PRIVATE glew_s

)

# ────────────────────────────────────────────────────────────────
#   Copy ONLY RenderX.dll → HelloTriangle
# ────────────────────────────────────────────────────────────────

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_BINARY_DIR}/test/HelloTriangle/$<CONFIG>"
    COMMENT "Copying RenderX.dll to HelloTriangle test project..."
)

