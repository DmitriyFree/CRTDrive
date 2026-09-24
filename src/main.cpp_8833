#include <Arduino.h>

constexpr int STBY_PIN = 17;
constexpr int AIN1_PIN = 18;
constexpr int AIN2_PIN = 19;

constexpr int PWM_FREQUENCY = 100000;
constexpr int PWM_RESOLUTION = 8;

constexpr int STEP_DELAY_MS = 10;

void setPositive(uint8_t duty)
{
    ledcWrite(AIN1_PIN, duty);
    ledcWrite(AIN2_PIN, 0);
}

void setNegative(uint8_t duty)
{
    ledcWrite(AIN1_PIN, 0);
    ledcWrite(AIN2_PIN, duty);
}

void off()
{
    ledcWrite(AIN1_PIN, 0);
    ledcWrite(AIN2_PIN, 0);
}

void setup()
{
    pinMode(STBY_PIN, OUTPUT);

    digitalWrite(STBY_PIN, HIGH);

    ledcAttach(AIN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttach(AIN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    off();
}

void loop()
{

        setPositive(255);
        delay(STEP_DELAY_MS);

        setNegative(255);
        delay(STEP_DELAY_MS);

        off();
        delay(STEP_DELAY_MS);
}