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

    print_stream << "DOUBLE:\n";
    using Types_d = Types<double, uint32_t, float>;
    perf::run_performance_suite<Types_d>(n_trials, time_func, print_stream);

    print_stream << "\n\n";
    
    print_stream << "FLOAT:\n";
    using Types_f = Types<float, uint32_t, float>;
    perf::run_performance_suite<Types_f>(n_trials, time_func, print_stream);
}