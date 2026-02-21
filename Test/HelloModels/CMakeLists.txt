cmake_minimum_required(VERSION 3.10)
project(HelloModle VERSION 1.0)

find_package(assimp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(HelloModle
    main.cpp
    ModelRenderer.cpp
    Files.cpp
)
target_compile_options(HelloModle
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)
target_link_libraries(HelloModle
    PRIVATE
    assimp::assimp
    RenderX
    glfw
)

