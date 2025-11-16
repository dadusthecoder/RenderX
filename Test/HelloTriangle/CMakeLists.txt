cmake_minimum_required(VERSION 3.10)

project(HelloTriangle VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────
#  Create executable
# ──────────────────────────────────────────────
add_executable(HelloTriangle Test.cpp)

# ──────────────────────────────────────────────
#  Include RenderX headers (from engine project)
# ──────────────────────────────────────────────
target_include_directories(HelloTriangle PRIVATE
    "${CMAKE_SOURCE_DIR}/Include"      # RenderX public headers
)

# ──────────────────────────────────────────────
#  Link with RenderX library built in /build/bin/<config>/
# ──────────────────────────────────────────────++











































target_link_directories(HelloTriangle PRIVATE
    "${CMAKE_SOURCE_DIR}/../../build/bin/${CMAKE_BUILD_TYPE}"
)

target_link_libraries(HelloTriangle PRIVATE RenderX)

# ──────────────────────────────────────────────
#  Output directory for test app
# ──────────────────────────────────────────────
set_target_properties(HelloTriangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
)
