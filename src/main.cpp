#include <Arduino.h>

constexpr int IN1_PIN    = 16;   // EN / PWM
constexpr int IN2_PIN    = 17;   // PH / direction
constexpr int NSLEEP_PIN = 5;
constexpr int NFAULT_PIN = 18;

constexpr int PWM_FREQ = 100000;
constexpr int PWM_BITS = 8;

void setPositive(uint8_t duty)
{
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(IN1_PIN, duty);
}

void setNegative(uint8_t duty)
{
    digitalWrite(IN2_PIN, HIGH);
    ledcWrite(IN1_PIN, duty);
}

void off()
{
    ledcWrite(IN1_PIN, 0);
}

void setup()
{
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(NSLEEP_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT_PULLUP);

    digitalWrite(NSLEEP_PIN, LOW);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);

    Serial.begin(115200);

    delay(500);

    digitalWrite(NSLEEP_PIN, HIGH);
    delay(100);

    ledcAttach(IN1_PIN, PWM_FREQ, PWM_BITS);

    Serial.print("nFAULT = ");
    Serial.println(digitalRead(NFAULT_PIN));
}

void loop()
{
    // Positive direction
    for (int duty = 0; duty <= 255; duty += 25)
    {
        Serial.print("Positive duty = ");
        Serial.println(duty);

        setPositive(duty);
        delay(1000);
    }

    off();
    delay(1000);

    // Negative direction
    for (int duty = 0; duty <= 255; duty += 25)
    {
        Serial.print("Negative duty = ");
        Serial.println(duty);

        setNegative(duty);
        delay(1000);
    }

    off();
    delay(2000);
}