#include <Arduino.h>

constexpr int STBY_PIN = 17;
constexpr int AIN1_PIN = 18;
constexpr int AIN2_PIN = 19;

constexpr int PWM_FREQUENCY = 10000;
constexpr int PWM_RESOLUTION = 8;

constexpr int MAX_DUTY = 80;
constexpr int SWING_TIME_MS = 2000;

void driveCoil(int duty)
{
    duty = constrain(duty, -255, 255);

    if (duty > 0)
    {
        ledcWrite(AIN1_PIN, duty);
        ledcWrite(AIN2_PIN, 0);
    }
    else if (duty < 0)
    {
        ledcWrite(AIN1_PIN, 0);
        ledcWrite(AIN2_PIN, -duty);
    }
    else
    {
        ledcWrite(AIN1_PIN, 0);
        ledcWrite(AIN2_PIN, 0);
    }
}

void setup()
{
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH);

    ledcAttach(AIN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttach(AIN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    driveCoil(0);
}

void loop()
{
    // Positive direction
    for (int duty = 0; duty <= MAX_DUTY; duty++)
    {
        driveCoil(duty);
        delay(SWING_TIME_MS / MAX_DUTY);
    }

    // Positive -> negative
    for (int duty = MAX_DUTY; duty >= -MAX_DUTY; duty--)
    {
        driveCoil(duty);
        delay((SWING_TIME_MS * 2) / (MAX_DUTY * 2));
    }

    // Negative -> zero
    for (int duty = -MAX_DUTY; duty <= 0; duty++)
    {
        driveCoil(duty);
        delay(SWING_TIME_MS / MAX_DUTY);
    }
}