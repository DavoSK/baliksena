cmake_minimum_required(VERSION 3.0)
project(baliksena)
set(CMAKE_CXX_STANDARD 23)

#NOTE(Zaklaus): setup build path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W4 /nologo)
endif()

add_subdirectory(vendors)
include_directories(vendors 
    src
    shaders/include
)

add_executable(baliksena 
    src/main.cpp
    src/app.cpp
    src/scene.cpp
    src/bmp_loader.cpp
    src/renderer.cpp
    src/input.cpp
    src/camera.cpp)

target_link_libraries(baliksena flecs)