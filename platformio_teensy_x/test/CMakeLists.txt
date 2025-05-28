# Keith Rausch

set(PROJ_NAME projects_codeonteensy)

set(CMAKE_CXX_STANDARD 20)

set(PROJ_NAME_TEST "${PROJ_NAME}_TEST")

enable_testing()

add_executable(
  ${PROJ_NAME_TEST}
  main_TEST.cpp
)
target_link_libraries(${PROJ_NAME_TEST} PUBLIC
#   ${PROJ_NAME}
    utils
    fftw3
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${PROJ_NAME_TEST})