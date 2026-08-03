set(TEENSY_VERSION 41 CACHE STRING "Teensy version: 40, 41, or 42 (RT1060-EVKB)" FORCE)
set(CPU_CORE_SPEED 600000000 CACHE STRING "CPU core speed in Hz" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs" CACHE INTERNAL "")   # needed if you link the C++ std lib
# set(COMPILERPATH "/Applications/ARM_10/bin/")                        # <-- edit for your machine
set(COMPILERPATH "/usr/bin/" CACHE STRING "Path to arm-none-eabi toolchain")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_C_COMPILER} <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")