# Keith Rausch

cmake_minimum_required(VERSION 3.24)
project(FFTWOnTeensy)

# Set compile warnings as errors default to on
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Set Link Time Optimizations default to true
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT error)
if(result)
    set(INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO/LTO not supported: <${error}>")
endif()

# function to print out all the directonies that a target is including
function(list_include_dirs LIB_NAME)
    # print out the included directories
    get_target_property(INCLUDE_DIRS ${LIB_NAME} INCLUDE_DIRECTORIES)
    foreach(INCLUDE_DIR IN LISTS INCLUDE_DIRS)
        # canonicanilize the include path and print it
        get_filename_component(INCLUDE_DIR_ABSOLUTE ${INCLUDE_DIR} ABSOLUTE)
        message(STATUS "  ${LIB_NAME} including: ${INCLUDE_DIR_ABSOLUTE}")
    endforeach()
endfunction()

# convenient macro to get a list of all sub directories
# be sure to understand the difference between cmake macros and functions
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child} AND EXISTS ${curdir}/${child}/CMakeLists.txt)
        LIST(APPEND dirlist ${child})
    ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


enable_testing()

add_subdirectory(lib)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)