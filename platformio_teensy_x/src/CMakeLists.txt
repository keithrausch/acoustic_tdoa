# Keith Rausch

set(PROJ_NAME projects_codeonteensy)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJ_NAME} main.cpp)

target_link_libraries(${PROJ_NAME} PUBLIC
    utils
    fftw3
)

# print included dirs
list_include_dirs(${PROJ_NAME})