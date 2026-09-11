#include <Arduino.h>

constexpr int PWM_PIN = 15;
constexpr int LED_PIN = 2;

constexpr int PWM_FREQUENCY = 1000;
constexpr int PWM_RESOLUTION = 16;
constexpr int STEP_TIME_MS = 200;

void setPwm(uint16_t duty)
{
    ledcWrite(PWM_PIN, duty);
    //ledcWrite(LED_PIN, duty);

    delay(STEP_TIME_MS);
}

void setup()
{
    Serial.begin(115200);

    ledcAttach(PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttach(LED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    Serial.println("PWM test started");

    delay(5000);
}

void loop()
{
    Serial.printf("%d Hz, 0.00%%\n", PWM_FREQUENCY);
    setPwm(0);

    Serial.printf("%d Hz, 0.02%%\n", PWM_FREQUENCY);
    setPwm(13);

    Serial.printf("%d Hz, 100.00%%\n", PWM_FREQUENCY);
    setPwm(65535);
}