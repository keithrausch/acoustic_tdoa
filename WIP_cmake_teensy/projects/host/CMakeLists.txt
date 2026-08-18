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

add_executable(projects_host_test main_test.cpp)
target_link_libraries(projects_host_test PRIVATE
    tdoa_utils
    fftw3 # TODO add flags to pick one of these
    # fftw3f # TODO add flags to pick one of these
    Eigen3::Eigen
    GTest::gtest_main
)
gtest_discover_tests(projects_host_test)


add_executable(projects_host_perf main_perf.cpp)
target_link_libraries(projects_host_perf PRIVATE
    tdoa_utils
    # Eigen3::Eigen
    # GTest::gtest_main
)
# gtest_discover_tests(projects_host_perf DISCOVERY_TIMEOUT 30)