cmake_minimum_required(VERSION 3.16)

project(vulkan_sandbox VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(MAKE_DIRECTORY "${SHADER_DIR}")

add_custom_command(
    OUTPUT "${SHADER_DIR}/triangle.vert.spv" "${SHADER_DIR}/triangle.frag.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_DIR}/triangle.vert" -o "${SHADER_DIR}/triangle.vert.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_DIR}/triangle.frag" -o "${SHADER_DIR}/triangle.frag.spv"
    DEPENDS "${SHADER_DIR}/triangle.vert" "${SHADER_DIR}/triangle.frag"
    COMMENT "Compiling Vulkan shaders"
    VERBATIM
)

add_executable(vulkan_sandbox vulkan_sandbox.cpp)
add_custom_target(vulkan_shaders DEPENDS "${SHADER_DIR}/triangle.vert.spv" "${SHADER_DIR}/triangle.frag.spv")
add_dependencies(vulkan_sandbox vulkan_shaders)

target_include_directories(vulkan_sandbox PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_sandbox PRIVATE glfw Vulkan::Vulkan)
