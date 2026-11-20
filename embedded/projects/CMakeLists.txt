
if(BUILD_HOST)
    add_subdirectory(host)
endif()

if(BUILD_TEENSY)
    add_subdirectory(teensy)
endif()