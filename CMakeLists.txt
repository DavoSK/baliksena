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

add_subdirectory(
    vendors/spdlog
)

include_directories(vendors 
    src
    shaders/include
    vendors/spdlog/include
)

add_executable(baliksena 
    src/mafia/parser_4ds.cpp
    src/mafia/parser_cachebin.cpp
    src/mafia/parser_scene2bin.cpp

    src/gui.cpp
    src/main.cpp
    src/app.cpp
    src/scene.cpp
    src/frame.cpp
    src/sector.cpp
    src/mesh.cpp
    src/billboard.cpp
    src/model.cpp
    src/bmp_loader.cpp
    src/model_loader.cpp
    src/renderer.cpp
    src/input.cpp
    src/camera.cpp
    src/frustum_culling.cpp
    src/texture.cpp 
    src/material.cpp
    src/logger.cpp
    vendors/imgui/imgui.cpp
    vendors/imgui/imgui_draw.cpp
    vendors/imgui/imgui_tables.cpp
    vendors/imgui/imgui_widgets.cpp
 )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(
        baliksena
        X11 
        Xi 
        Xcursor 
        GL 
        dl 
        pthread 
        m
    )
else()
    target_link_libraries(baliksena)    
endif()