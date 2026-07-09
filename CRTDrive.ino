/*
    ESP32 + DRV8833 CRT Deflection Demo
    -----------------------------------

    Arduino ESP32 Core 3.x

    Sign-magnitude drive

    X+ -> AIN1
    X- -> AIN2

    Y+ -> BIN1
    Y- -> BIN2
*/

#include <Arduino.h>

// ---------------- Pins ----------------

const int X_POS_PIN = 25;
const int X_NEG_PIN = 26;

const int Y_POS_PIN = 27;
const int Y_NEG_PIN = 14;

// -------------- PWM Setup --------------

const uint32_t PWM_FREQ = 50000;     // 50 kHz
const uint8_t PWM_BITS = 10;

const uint16_t PWM_MAX = (1 << PWM_BITS) - 1;

// ---------------- Gain -----------------

float gainX = 1.0f;
float gainY = 1.0f;

float amplitude = 0.80f;
float speed = 0.60f;

// ---------------------------------------

void writeBridge(int pinPos, int pinNeg, float value)
{
    value = constrain(value, -1.0f, 1.0f);

    uint16_t duty = (uint16_t)(fabsf(value) * PWM_MAX);

    if (value >= 0.0f)
    {
        ledcWrite(pinPos, duty);
        ledcWrite(pinNeg, 0);
    }
    else
    {
        ledcWrite(pinPos, 0);
        ledcWrite(pinNeg, duty);
    }
}

void setXY(float x, float y)
{
    writeBridge(X_POS_PIN, X_NEG_PIN, x * gainX);
    writeBridge(Y_POS_PIN, Y_NEG_PIN, y * gainY);
}

void setup()
{
    ledcAttach(X_POS_PIN, PWM_FREQ, PWM_BITS);
    ledcAttach(X_NEG_PIN, PWM_FREQ, PWM_BITS);

    ledcAttach(Y_POS_PIN, PWM_FREQ, PWM_BITS);
    ledcAttach(Y_NEG_PIN, PWM_FREQ, PWM_BITS);

    setXY(0.0f, 0.0f);
}

void loop()
{
    float t = millis() * 0.001f * speed;

    float x = sinf(t) * amplitude;
    float y = cosf(t) * amplitude;

    setXY(x, y);

    delayMicroseconds(250);
}