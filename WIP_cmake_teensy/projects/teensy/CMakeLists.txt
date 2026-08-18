cmake_minimum_required(VERSION 3.20)

# project(projects_teensy_ethernet_sandbox C CXX ASM)

include(FetchContent)


#
# teensy cmake macros
#
FetchContent_Declare(
    teensy_cmake_macros
    GIT_REPOSITORY https://github.com/newdigate/teensy-cmake-macros.git
    GIT_TAG main
)

FetchContent_MakeAvailable(teensy_cmake_macros)

include(${teensy_cmake_macros_SOURCE_DIR}/CMakeLists.include.txt)

# teensy core
import_arduino_library(
    cores
    ${teensy_cores_SOURCE_DIR}/teensy4
    avr
    util
)

# spi
import_arduino_library_git(
    SPI
    https://github.com/PaulStoffregen/SPI.git
    master
    ""
)


#
# QNEthernet (not a native Arduino project so requires a lot of hand-holding)
#
FetchContent_Declare(
    QNEthernet
    GIT_REPOSITORY https://github.com/ssilverman/QNEthernet.git
    GIT_TAG v0.36.0
)

FetchContent_MakeAvailable(QNEthernet)

file(GLOB_RECURSE QNET_SOURCES CONFIGURE_DEPENDS
    ${qnethernet_SOURCE_DIR}/src/*.cpp
    ${qnethernet_SOURCE_DIR}/src/*.c
)

teensy_add_library(QNEthernet 
    ${QNET_SOURCES}
)

target_include_directories(QNEthernet.o PUBLIC
    ${qnethernet_SOURCE_DIR}/src
    ${qnethernet_SOURCE_DIR}/src/lwip
)

target_compile_definitions(QNEthernet.o PUBLIC
    LWIP_PROVIDE_ERRNO
)

#
# executables
#

# ethernet sandbox
teensy_add_executable(projects_teensy_ethernet_sandbox main_ethernet.cpp)
teensy_target_link_libraries(projects_teensy_ethernet_sandbox
    cores
    SPI
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_ethernet_sandbox.elf stdc++)


# perf
teensy_add_executable(projects_teensy_perf main_perf.cpp)
target_link_libraries(projects_teensy_perf.elf tdoa_utils)
teensy_target_link_libraries(projects_teensy_perf
    cores
    SPI
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_perf.elf stdc++)