#ifndef SERIAL_STREAM_HPP
#define SERIAL_STREAM_HPP

#include "Wire.h"

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

#endif