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

if(MSVC)
add_subdirectory(
   vendors/openal-soft
)
endif()

include_directories(vendors 
    src
    shaders/include
    vendors/spdlog/include
    vendors/openal-soft/include
    vendors/imgui
    vendors/imguizmo
    vendors/dawn/inc
)

add_executable(baliksena 
    src/mafia/parser_5ds.cpp
    src/mafia/parser_4ds.cpp
    src/mafia/parser_cachebin.cpp
    src/mafia/parser_scene2bin.cpp
    src/mafia/parser_dta.cpp
    src/mafia/utils.cpp
    
    src/vfs.cpp
    src/gui.cpp
    src/main.cpp
    src/app.cpp
    src/scene.cpp
    src/frame.cpp
    src/sound.cpp
    src/audio.cpp
    src/sector.cpp
    src/mesh.cpp
    src/single_mesh.cpp
    src/animator.cpp
    src/billboard.cpp
    src/model.cpp
    src/light.cpp
    src/lit_object.cpp
    src/bmp_loader.cpp
    src/model_loader.cpp
    src/renderer.cpp
    src/input.cpp
    src/camera.cpp
    src/bounding_volumes.cpp
    src/frustum_culling.cpp
    src/texture.cpp 
    src/material.cpp
    src/logger.cpp

    vendors/imgui/imgui.cpp
    vendors/imgui/imgui_draw.cpp
    vendors/imgui/imgui_tables.cpp
    vendors/imgui/imgui_widgets.cpp

    vendors/imguizmo/ImGuizmo.cpp
    vendors/imguizmo/ImSequencer.cpp
    vendors/imguizmo/ImGradient.cpp
    vendors/imguizmo/ImCurveEdit.cpp
    vendors/imguizmo/GraphEditor.cpp
)

if (UNIX)
    find_package(OpenAL REQUIRED)
endif()

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
        openal
    )
else()
    target_link_libraries(baliksena OpenAL)    
endif()