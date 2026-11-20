#ifndef TRNG
#define TRNG

#include <Arduino.h>
#include <imxrt.h>

// for teensy 4.1 hardware TRNG

uint32_t trng_random()
{
    return TRNG_ENT0;
}

#endif