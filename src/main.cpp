#include <Arduino.h>

constexpr int IN1_PIN    = 16;   // EN / PWM
constexpr int IN2_PIN    = 17;   // PH / direction
constexpr int NSLEEP_PIN = 5;
constexpr int NFAULT_PIN = 18;

constexpr int PWM_FREQ = 100000;
constexpr int PWM_BITS = 8;

// Time between amplitude updates.
// Increase this if you want slower motion.
constexpr int STEP_DELAY_MS = 0;

void setCurrent(int value)
{
    // value: -255 ... +255

    if (value > 0)
    {
        digitalWrite(IN2_PIN, LOW);
        ledcWrite(IN1_PIN, value);
    }
    else if (value < 0)
    {
        digitalWrite(IN2_PIN, HIGH);
        ledcWrite(IN1_PIN, -value);
    }
    else
    {
        ledcWrite(IN1_PIN, 0);
    }
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
    // -255 → +255
    for (int current = -150; current <= 150; current++)
    {
        setCurrent(current);
        delay(STEP_DELAY_MS);
    }

    // +255 → -255
    for (int current = 150; current >= -150; current--)
    {
        setCurrent(current);
        delay(STEP_DELAY_MS);
    }
}