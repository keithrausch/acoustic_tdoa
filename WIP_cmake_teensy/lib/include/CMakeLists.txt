# Keith Rausch

set(LIB_NAME utils)
set(LIB_ROOT "../../")

set(CMAKE_CXX_STANDARD 20)

# all headers
set(HEADERS_${LIB_NAME}
    constants.hpp
    domain.hpp
    types.hpp
    utils.hpp
    )

# all source files
set(SRC_${LIB_NAME}
    # detector.cpp
    )

# set up the library we are building
add_library(${LIB_NAME} SHARED ${HEADERS_${LIB_NAME}} ${SRC_${LIB_NAME}})
set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE          CXX
                                                COMPILE_WARNING_AS_ERROR ON)


# target_include_directories(${LIB_NAME} PUBLIC "../../lib/fftw/fftw-3.3.10")
target_include_directories(${LIB_NAME} PUBLIC ".")
target_include_directories(${LIB_NAME} PUBLIC "${LIB_ROOT}")
# target_include_directories(${LIB_NAME} PUBLIC "${LIB_ROOT}/lib/eigen/eigen")

# target_link_libraries(${LIB_NAME} PUBLIC
#     lib_common
#     )


# print included dirs
# list_include_dirs(${LIB_NAME})
