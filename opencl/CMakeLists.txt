cmake_minimum_required(VERSION 3.16)

project(opencl_fft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCL REQUIRED)

find_path(OpenCL_INCLUDE_DIR
    NAMES CL/cl.h
    HINTS
        /System/Library/Frameworks/OpenCL.framework/Headers
        /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX27.0.sdk/System/Library/Frameworks/OpenCL.framework/Headers
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

if(OpenCL_INCLUDE_DIR)
    set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIR})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fft_kernel.cl
               ${CMAKE_CURRENT_BINARY_DIR}/fft_kernel.cl
               COPYONLY)

add_executable(fft_opencl
    fft_opencl.cpp
)

if(NOT APPLE)
    target_include_directories(fft_opencl PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()

target_link_libraries(fft_opencl PRIVATE ${OpenCL_LIBRARIES})

if(APPLE)
    target_link_libraries(fft_opencl PRIVATE "-framework OpenCL")
endif()
