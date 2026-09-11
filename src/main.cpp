#include <Arduino.h>

constexpr int STBY_PIN = 17;
constexpr int AIN1_PIN = 18;
constexpr int AIN2_PIN = 19;

constexpr int PWM_FREQUENCY = 10000;
constexpr int PWM_RESOLUTION = 8;

constexpr uint8_t PWM_DUTY = 255;

void positive()
{
    // Forward PWM, slow decay
    ledcWrite(AIN1_PIN, PWM_DUTY);
    ledcWrite(AIN2_PIN, 0);
}

void negative()
{
    // Reverse PWM, slow decay
    ledcWrite(AIN1_PIN, 0);
    ledcWrite(AIN2_PIN, PWM_DUTY);
}

void off()
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

    off();
}

void loop()
{
    positive();
    delay(2000);

    off();
    delay(1000);

    negative();
    delay(2000);

    off();
    delay(1000);
}