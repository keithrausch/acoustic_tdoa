cmake_minimum_required(VERSION 3.20)

project(tdoa_project
    VERSION 1.0
    LANGUAGES C CXX ASM
)

# globals
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# build options
option(BUILD_HOST
    "Build host code (unit tests, perf tests, etc)"
    OFF
)

option(BUILD_TEENSY
    "Build Teensy firmware"
    OFF
)

include(CTest)

add_subdirectory(lib)
add_subdirectory(projects)