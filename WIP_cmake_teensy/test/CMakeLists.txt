find_package(GTest REQUIRED)

include(GoogleTest)
include(FetchContent)

FetchContent_Declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Eigen)

add_executable(utils_tests main_TEST.cpp)
target_link_libraries(utils_tests PRIVATE
    utils
    fftw3 # TODO add flags to pick one of these
    # fftw3f # TODO add flags to pick one of these
    Eigen3::Eigen
    GTest::gtest_main
)
gtest_discover_tests(utils_tests)
target_compile_features(utils_tests PRIVATE cxx_std_23)


add_executable(perf_tests main_perf.cpp)
target_link_libraries(perf_tests PRIVATE
    utils
    Eigen3::Eigen
    GTest::gtest_main
)
target_compile_features(perf_tests PRIVATE cxx_std_23)
gtest_discover_tests(perf_tests DISCOVERY_TIMEOUT 30)