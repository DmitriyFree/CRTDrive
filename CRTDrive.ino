/*
    ESP32 + 4x DRV8833 CRT Deflection Demo
    --------------------------------------

    Arduino ESP32 Core 3.x

    One DRV8833 per coil.

    TOP:
        TOP+ -> AIN1
        TOP- -> AIN2

    BOTTOM:
        BOT+ -> AIN1
        BOT- -> AIN2

    LEFT:
        LFT+ -> AIN1
        LFT- -> AIN2

    RIGHT:
        RGT+ -> AIN1
        RGT- -> AIN2
*/

#include <Arduino.h>

const uint32_t PWM_FREQ = 50000;
const uint8_t PWM_BITS = 10;
const uint16_t PWM_MAX = (1 << PWM_BITS) - 1;

//----------------------------------------------------
// Pin assignment
//----------------------------------------------------

// Top
const int TOP_POS = 2;
const int TOP_NEG = 4;

// Bottom
const int BOT_POS = 16;
const int BOT_NEG = 17;

// Left
const int LFT_POS = 25;
const int LFT_NEG = 26;

// Right
const int RGT_POS = 27;
const int RGT_NEG = 14;

//----------------------------------------------------
// Calibration
//----------------------------------------------------

float gainTop    = 1.0f;
float gainBottom = 1.0f;
float gainLeft   = 1.0f;
float gainRight  = 1.0f;

float amplitude = 0.8f;
float speed = 0.6f;

//----------------------------------------------------

void writeBridge(int pinPos, int pinNeg, float value)
{
    value = constrain(value, -1.0f, 1.0f);

    uint16_t duty = fabsf(value) * PWM_MAX;

    if(value >= 0)
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

void setCoils(float top,
              float bottom,
              float left,
              float right)
{
    writeBridge(TOP_POS, TOP_NEG, top * gainTop);
    writeBridge(BOT_POS, BOT_NEG, bottom * gainBottom);

    writeBridge(LFT_POS, LFT_NEG, left * gainLeft);
    writeBridge(RGT_POS, RGT_NEG, right * gainRight);
}

void setXY(float x, float y)
{
    x = constrain(x, -1.0f, 1.0f);
    y = constrain(y, -1.0f, 1.0f);

    // Differential drive

    setCoils(
        +y,     // top
        -y,     // bottom

        +x,     // left
        -x      // right
    );
}

void attachPin(int pin)
{
    ledcAttach(pin, PWM_FREQ, PWM_BITS);
}

void setup()
{
    attachPin(TOP_POS);
    attachPin(TOP_NEG);

    attachPin(BOT_POS);
    attachPin(BOT_NEG);

    attachPin(LFT_POS);
    attachPin(LFT_NEG);

    attachPin(RGT_POS);
    attachPin(RGT_NEG);

    setCoils(0,0,0,0);
}

void loop()
{
    float t = millis() * 0.001f * speed;

    float x = sinf(t) * amplitude;
    float y = cosf(t) * amplitude;

    setXY(x,y);

    delayMicroseconds(250);
}