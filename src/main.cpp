#include <Arduino.h>

constexpr int PWM_PIN = 15;
constexpr int PWM_CHANNEL = 0;
constexpr int PWM_FREQUENCY = 1000;
constexpr int PWM_RESOLUTION = 8;

void setPwm(uint8_t duty)
{
    ledcWrite(PWM_PIN, duty);
    delay(5000);
}

void setup()
{
    Serial.begin(115200);

    ledcAttach(PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    Serial.println("PWM test started");
}

void loop()
{
    Serial.println("100 kHz, 50%");
    setPwm(128);

    Serial.println("100 kHz, 10%");
    setPwm(26);

    Serial.println("100 kHz, 100%");
    setPwm(255);

    Serial.println("100 kHz, 80%");
    setPwm(204);
}