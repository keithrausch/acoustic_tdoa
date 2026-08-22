#include "perf.hpp"
#include <chrono>
#include <iostream>


int main()
{
    auto time_func = []() -> uint64_t { 
        // return std::chrono::high_resolution_clock::now(); 
        return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());
    };

    perf::StreamWrapper print_stream{.permit_writes=true, .stream=std::cout};
    // perf::NullStream type available too

    constexpr size_t n_trials = 10;
    perf::run_performance_suite(n_trials, time_func, print_stream);
}