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

# I2C / Wire
import_arduino_library_git(
    Wire
    https://github.com/PaulStoffregen/Wire.git
    master
    ""
    utility
)

# SD card (required for Audio library)
import_arduino_library_git(
    SD
    https://github.com/PaulStoffregen/SD.git
    master
    ""
)
target_compile_options(SD.o PRIVATE
    -Wno-overloaded-virtual
    -Wno-volatile
    -Wno-class-memaccess
)

# Serial Flash
import_arduino_library_git(
    SerialFlash
    https://github.com/PaulStoffregen/SerialFlash.git
    master
    ""
)


# Teensy Audio library
import_arduino_library_git(
    Audio
    https://github.com/PaulStoffregen/Audio.git
    master
    ""
    utility
)

target_include_directories(Audio.o PUBLIC SYSTEM
    ${wire_SOURCE_DIR}
    ${wire_SOURCE_DIR}/utility
    ${sd_SOURCE_DIR}
    ${serialflash_SOURCE_DIR}
)
target_compile_options(Audio.o PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-overloaded-virtual>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
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

target_include_directories(QNEthernet.o PUBLIC SYSTEM
    ${qnethernet_SOURCE_DIR}/src
    ${qnethernet_SOURCE_DIR}/src/lwip
)

target_compile_definitions(QNEthernet.o PUBLIC
    LWIP_PROVIDE_ERRNO
)

#
# functions
#
function(set_teensy_cxx23 target)
    set(elf_target "${target}.elf")

    # Replace the Teensy macro's C++17 standard flag.
    get_target_property(_opts teensy_flags INTERFACE_COMPILE_OPTIONS)

    if(NOT _opts)
        set(_opts "")
    endif()

    list(FILTER _opts EXCLUDE REGEX "^-std=gnu\\+\\+17$")
    list(APPEND _opts "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>")

    set_target_properties(teensy_flags PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${_opts}"
    )
endfunction()

#
#
# executables
#
#

#
# ethernet sandbox
#
teensy_add_executable(projects_teensy_ethernet_sandbox main_ethernet.cpp)
set_teensy_cxx23(projects_teensy_ethernet_sandbox)
teensy_target_link_libraries(projects_teensy_ethernet_sandbox
    cores
    SPI
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_ethernet_sandbox.elf stdc++)


#
# perf
#
teensy_add_executable(projects_teensy_perf main_perf.cpp)
set_teensy_cxx23(projects_teensy_perf)
target_link_libraries(projects_teensy_perf.elf tdoa_utils)
teensy_target_link_libraries(projects_teensy_perf
    cores
    SPI
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_perf.elf stdc++)


#
# 4channel
#
teensy_add_executable(projects_teensy_4channel main_4channel.hpp main_4channel.cpp)
set_teensy_cxx23(projects_teensy_4channel)
target_link_libraries(projects_teensy_4channel.elf tdoa_utils)
teensy_target_link_libraries(projects_teensy_4channel
    cores
    SPI
    Wire # needed for Audio
    SD # needed for Audio
    SerialFlash # needed for Audio
    Audio
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_4channel.elf stdc++)


#
# emitter
#
teensy_add_executable(projects_teensy_emitter main_emitter.cpp)
set_teensy_cxx23(projects_teensy_emitter)
target_link_libraries(projects_teensy_emitter.elf tdoa_utils)
teensy_target_link_libraries(projects_teensy_emitter
    cores
    SPI
    Wire # needed for Audio
    SD # needed for Audio
    SerialFlash # needed for Audio
    Audio
    QNEthernet
)
# set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "") already provided by teensy-cmake-macros?
target_link_libraries(projects_teensy_emitter.elf stdc++)
