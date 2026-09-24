#include <Arduino.h>

// DRV8876 pins
constexpr int IN1_PIN    = 18;  // PWM / EN
constexpr int IN2_PIN    = 19;  // PH / direction
constexpr int NSLEEP_PIN = 17;

constexpr int NFAULT_PIN = 16; // optional
constexpr int CS_PIN     = 34; // optional ADC input

constexpr int PWM_FREQUENCY = 100000;
constexpr int PWM_RESOLUTION = 8;

constexpr int STEP_DELAY_MS = 10;


// ------------------------------------------------------------
// Coil control
// ------------------------------------------------------------

void setPositive(uint8_t duty)
{
    // PH = 1
    digitalWrite(IN2_PIN, HIGH);

    // EN = PWM
    ledcWrite(IN1_PIN, duty);
}

void setNegative(uint8_t duty)
{
    // PH = 0
    digitalWrite(IN2_PIN, LOW);

    // EN = PWM
    ledcWrite(IN1_PIN, duty);
}

void off()
{
    ledcWrite(IN1_PIN, 0);
}


// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup()
{
    pinMode(NSLEEP_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);

    pinMode(NFAULT_PIN, INPUT_PULLUP);

    digitalWrite(NSLEEP_PIN, HIGH);

    // IN1 is the PWM input
    ledcAttach(IN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

    off();
}


// ------------------------------------------------------------
// Test
// ------------------------------------------------------------

void loop()
{
    setPositive(50);
    delay(STEP_DELAY_MS);

    setNegative(50);
    delay(STEP_DELAY_MS);

    off();
    delay(STEP_DELAY_MS);
}