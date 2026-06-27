cmake_minimum_required(VERSION 3.16)
project(gui_multiwindow_anim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(gui_multiwindow_anim multiwindow_anim.cpp)

find_package(SDL2 QUIET CONFIG)

if(SDL2_FOUND)
    target_link_libraries(gui_multiwindow_anim PRIVATE SDL2::SDL2)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)

    target_include_directories(gui_multiwindow_anim PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(gui_multiwindow_anim PRIVATE ${SDL2_LIBRARY_DIRS})
    target_compile_options(gui_multiwindow_anim PRIVATE ${SDL2_CFLAGS_OTHER})
    target_link_libraries(gui_multiwindow_anim PRIVATE ${SDL2_LIBRARIES})
endif()
