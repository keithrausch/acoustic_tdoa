cmake_minimum_required(VERSION 3.20)

project(my_firmware C CXX ASM)

include(FetchContent)


#
# Teensy CMake macros
#
FetchContent_Declare(
    teensy_cmake_macros
    GIT_REPOSITORY https://github.com/newdigate/teensy-cmake-macros.git
    GIT_TAG main
)

FetchContent_MakeAvailable(teensy_cmake_macros)

include(${teensy_cmake_macros_SOURCE_DIR}/CMakeLists.include.txt)

# Teensy core
import_arduino_library(
    cores
    ${teensy_cores_SOURCE_DIR}/teensy4
    avr
    util
)

# SPI library
import_arduino_library_git(
    SPI
    https://github.com/PaulStoffregen/SPI.git
    master
    ""
)


#
# QNEthernet source
#
FetchContent_Declare(
    QNEthernet
    GIT_REPOSITORY https://github.com/ssilverman/QNEthernet.git
    GIT_TAG v0.36.0
)

FetchContent_MakeAvailable(QNEthernet)


#
# Gather every source file in the library
#
file(GLOB_RECURSE QNET_SOURCES CONFIGURE_DEPENDS
    ${qnethernet_SOURCE_DIR}/src/*.cpp
    ${qnethernet_SOURCE_DIR}/src/*.c
)


#
# Build QNEthernet as a Teensy library
#
teensy_add_library(QNEthernet
    ${QNET_SOURCES}
)


#
# QNEthernet includes
#
target_include_directories(QNEthernet.o PUBLIC
    ${qnethernet_SOURCE_DIR}/src
    ${qnethernet_SOURCE_DIR}/src/lwip
)


#
# QNEthernet-specific definitions only
#
target_compile_definitions(QNEthernet.o PUBLIC
    LWIP_PROVIDE_ERRNO
)

#
# Application
#
teensy_add_executable(my_firmware
    main_ethernet.cpp
)

teensy_target_link_libraries(my_firmware
    cores
    SPI
    QNEthernet
)

# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(my_firmware.elf
    stdc++
)