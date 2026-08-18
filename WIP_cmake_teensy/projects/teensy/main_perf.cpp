#include <Arduino.h>
#include "perf.hpp"

struct ArduinoSerialStream
{
    template <typename T>
    ArduinoSerialStream& operator<<(const T& value)
    {
        Serial.print(value);
        return *this;
    }

    ArduinoSerialStream& operator<<(const std::string& value)
    {
        Serial.print(value.c_str());
        return *this;
    }
};


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
    perf::run_performance_suite(n_trials, time_func, print_stream);
}