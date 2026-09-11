#include <Arduino.h>
#include "PwmService.h"

constexpr int PWM_PIN = 15;
constexpr int PWM_FREQUENCY = 1000;
constexpr int PWM_RESOLUTION = 16;

PwmService pwm(PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

void setup()
{
    pwm.begin();
}

void loop()
{
    pwm.setDuty(0);
    delay(50);

    pwm.setDuty(65535);
    delay(50);
}