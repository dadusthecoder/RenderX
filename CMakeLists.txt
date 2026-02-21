cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE 
        "D:/dev/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING ""
    )
endif()

project(RenderX VERSION 1.0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-int")
set(CMAKE_OBJECT_PATH_MAX 256)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG}/${PROJECT_NAME}")
    set(CMAKE_OBJECT_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin-int/${CONFIG}/${PROJECT_NAME}")
endforeach()

option(RX_BUILD_VULKAN "Enable Vulkan backend" ON)
option(RX_BUILD_OPENGL "Enable OpenGL backend" OFF) 
option(RX_BUILD_DLL "Build Shared" ON)
option(RX_BUILD_TESTS "Build Tests" ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Building 64-bit")
else()
    message(FATAL_ERROR "RenderX requires 64-bit build.")
endif()

add_subdirectory(External/glm)
add_subdirectory(External/spdlog)
add_subdirectory(External_local/ProLog)

if(RX_BUILD_TESTS)
    add_subdirectory(Test/HelloCube)
    add_subdirectory(Test/HelloModels)
endif()

file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RenderX/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RenderX/*.h"
)

if(NOT RX_BUILD_VULKAN AND NOT RX_BUILD_OPENGL)
    message(FATAL_ERROR "At least one graphics backend must be enabled.")
endif()

set(SOURCES ${CORE_SOURCES})

if(RX_BUILD_VULKAN)
    file(GLOB_RECURSE VULKAN_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.h"
    )
    list(APPEND SOURCES ${VULKAN_SOURCES})
endif()

if(RX_BUILD_OPENGL)
    file(GLOB_RECURSE OPENGL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenGL/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenGL/*.h"
    )
    list(APPEND SOURCES ${OPENGL_SOURCES})
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RX_PLATFORM_WINDOWS)
    message(STATUS "Building for Windows")
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RX_PLATFORM_LINUX)
    message(STATUS "Building for Linux")
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RX_PLATFORM_MACOS)
    message(STATUS "Building for macOS")
endif()

if(RX_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(RenderX PRIVATE Vulkan::Vulkan)
    target_compile_definitions(RenderX PRIVATE RX_ENABLE_VULKAN)
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE
            VK_USE_PLATFORM_WIN32_KHR
        )
    elseif(UNIX AND NOT APPLE)
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE
            VK_USE_PLATFORM_XLIB_KHR
        )
    endif()
endif()

if(RX_BUILD_OPENGL)
    find_package(OpenGL REQUIRED)
    target_link_libraries(RenderX PRIVATE OpenGL::GL)
    target_compile_definitions(RenderX PRIVATE RX_ENABLE_OPENGL)
endif()

if(RX_BUILD_DLL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RX_BUILD_DLL)
endif()

target_compile_definitions(RenderX PRIVATE
    $<$<CONFIG:Debug>:RX_DEBUG_BUILD>
    $<$<CONFIG:Release>:RX_RELEASE_BUILD>
)

target_compile_options(RenderX PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_directories(RenderX PUBLIC External_local/ProLog/bin)

target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/External/VulkanMemoryAllocator/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/External/spdlog/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/External/glm"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/External_local/ProLog/include"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE glm
    PUBLIC spdlog
    PUBLIC ProLog
)


