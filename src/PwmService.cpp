#include "PwmService.h"

PwmService::PwmService(int pin, uint32_t frequency, uint8_t resolution)
{
    _pin = pin;
    _frequency = frequency;
    _resolution = resolution;
}

void PwmService::begin()
{
    ledcAttach(_pin, _frequency, _resolution);
}

void PwmService::setDuty(uint16_t duty)
{
    ledcWrite(_pin, duty);
}