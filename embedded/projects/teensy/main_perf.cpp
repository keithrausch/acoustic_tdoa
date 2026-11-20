#include <Arduino.h>
#include "perf.hpp"
#include "serial_stream.hpp"


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println("Starting ...");
}

void loop()
{

    auto time_func = []() { return micros(); };

    ArduinoSerialStream serial_stream{};
    perf::StreamWrapper print_stream{.permit_writes=true, .stream=serial_stream};

    constexpr size_t n_trials = 10;

    print_stream << "DOUBLE:\n";
    using Types_d = Types<double, uint32_t, float>;
    perf::run_performance_suite<Types_d>(n_trials, time_func, print_stream);

    print_stream << "\n\n";
    
    print_stream << "FLOAT:\n";
    using Types_f = Types<float, uint32_t, float>;
    perf::run_performance_suite<Types_f>(n_trials, time_func, print_stream);
}