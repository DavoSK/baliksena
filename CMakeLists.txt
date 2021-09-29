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
endif()

include_directories(vendors 
    src
    shaders/include
)

add_executable(baliksena 
    src/mafia/parser_4ds.cpp
    src/mafia/parser_cachebin.cpp
    src/mafia/parser_scene2bin.cpp

    src/main.cpp
    src/app.cpp
    src/scene.cpp
    src/frame.cpp
    src/mesh.cpp
    src/model.cpp
    src/bmp_loader.cpp
    src/model_loader.cpp
    src/renderer.cpp
    src/input.cpp
    src/camera.cpp
    src/texture.cpp src/material.cpp src/material.hpp)

target_link_libraries(baliksena)