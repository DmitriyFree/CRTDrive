#pragma once

#include <Arduino.h>

class PwmService
{
public:
    PwmService(int pin, uint32_t frequency, uint8_t resolution);

    void begin();
    void setDuty(uint16_t duty);

private:
    int _pin;
    uint32_t _frequency;
    uint8_t _resolution;
};