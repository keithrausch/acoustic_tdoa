# Keith Rausch

set(LIB_NAME tdoa_utils)

# all headers
set(HEADERS_${LIB_NAME}
    constants.hpp
    domain.hpp
    fft.hpp
    perf.hpp
    types.hpp
    utils.hpp
    )

# all source files
set(SRC_${LIB_NAME}
    # no .cpp yet
    )

# set up the library we are building
add_library(${LIB_NAME} STATIC ${HEADERS_${LIB_NAME}} ${SRC_${LIB_NAME}})
set_target_properties(${LIB_NAME} PROPERTIES 
    LINKER_LANGUAGE          CXX
    COMPILE_WARNING_AS_ERROR ON
)

target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_options(tdoa_utils PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-psabi
)