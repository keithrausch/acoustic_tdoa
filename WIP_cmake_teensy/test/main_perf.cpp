#include "utils_perf.hpp"
#include <chrono>

template <typename TestFuncT>
void run_test(const TestFuncT & test_func)
{
    constexpr size_t n_trials =  100;

    std::chrono::microseconds total_duration{};

    for (size_t i = 0; i < n_trials; ++i)
    {
        auto time_delta = test_func();
        total_duration += std::chrono::duration_cast<std::chrono::microseconds>(time_delta);
    }
    std::cout << "time_delta: " << total_duration.count()/n_trials << "μs\n";
}


int main()
{
    auto time_func = []() { return std::chrono::high_resolution_clock::now(); };

    run_test([&](){return naive_c2c_live_exp(time_func);});
    run_test([&](){return naive_r2c_live_exp(time_func);});
    run_test([&](){return niave_r2c_precompute_exp(time_func);});
    run_test([&](){return fft_r2c_radix2(time_func);});
}