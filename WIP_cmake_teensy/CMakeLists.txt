cmake_minimum_required(VERSION 3.20)

project(my_firmware
    VERSION 1.0
    LANGUAGES C CXX ASM
)

# ---------------------------------------------------------------------------
# Global C++ configuration
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

option(BUILD_TESTING
    "Build host unit tests"
    OFF
)

option(BUILD_TEENSY
    "Build Teensy firmware"
    OFF
)

# ---------------------------------------------------------------------------
# Testing infrastructure
#
# This defines BUILD_TESTING and enables CTest when requested.
# ---------------------------------------------------------------------------

include(CTest)

# ---------------------------------------------------------------------------
# Portable application/library code
#
# These targets must not depend on Arduino/Teensy-specific headers or
# libraries if you want to build them natively on the host.
# ---------------------------------------------------------------------------

add_subdirectory(lib)

# ---------------------------------------------------------------------------
# Host unit tests
#
# This directory is only entered for a native/host build.
# ---------------------------------------------------------------------------

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

# ---------------------------------------------------------------------------
# Teensy firmware
#
# All Teensy-specific CMake code, Arduino libraries, QNEthernet, etc.
# lives underneath teensy/.
# ---------------------------------------------------------------------------

if(BUILD_TEENSY)
    add_subdirectory(teensy)
endif()