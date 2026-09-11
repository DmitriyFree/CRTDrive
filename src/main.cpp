#include <Arduino.h>

constexpr int STBY_PIN = 17;
constexpr int AIN1_PIN = 18;
constexpr int AIN2_PIN = 19;

constexpr int PWM_FREQUENCY = 1000;
constexpr int PWM_RESOLUTION = 8;

constexpr uint8_t PWM_DUTY = 16;
constexpr uint32_t PHASE_TIME_MS = 1000;

void positive()
{
    ledcWrite(AIN1_PIN, PWM_DUTY);
    digitalWrite(AIN2_PIN, 0);
}

void negative()
{
    ledcWrite(AIN1_PIN, 0);
    ledcWrite(AIN2_PIN, PWM_DUTY);
}

void zero()
{
    ledcWrite(AIN1_PIN, 0);
    ledcWrite(AIN2_PIN, 0);
}

void setup()
{
    pinMode(STBY_PIN, OUTPUT);
    pinMode(AIN1_PIN, OUTPUT);
    pinMode(AIN2_PIN, OUTPUT);

    digitalWrite(STBY_PIN, HIGH);

    ledcAttach(AIN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttach(AIN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    zero();
}

void loop()
{
    // + PWM for 1 second
    positive();
    delay(PHASE_TIME_MS);

    // 0 V for 1 second
    zero();
    delay(PHASE_TIME_MS);

    // - PWM for 1 second
    negative();
    delay(PHASE_TIME_MS);

    // 0 V for 1 second
    zero();
    delay(PHASE_TIME_MS);
}