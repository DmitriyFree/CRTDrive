#include <Arduino.h>

const uint32_t PWM_FREQ = 150000;
const uint8_t PWM_BITS = 8;
const uint16_t PWM_MAX = (1 << PWM_BITS) - 1;

//----------------------------------------------------
// Pin assignment
//----------------------------------------------------

// Top
const int LFT_POS = 2;
const int LFT_NEG = 4;

// Bottom
const int TOP_POS = 16;
const int TOP_NEG = 17;

// Left
const int RGT_POS = 25;
const int RGT_NEG = 26;

// Right
const int BOT_POS = 27;
const int BOT_NEG = 14;

const int COILS_ENABLE = 5;
const int ALPHA_CHANNEL = 18;

//----------------------------------------------------
// Calibration
//----------------------------------------------------

float gainTop    = 1.0f;
float gainBottom = 1.0f;
float gainLeft   = 1.0f;
float gainRight  = 1.0f;

float amplitude = 1.0f;
float speed = 1.0f;

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
void writeBridgeSimple(int pinPos, int pinNeg, float value)
{
    if (value > 0)
    {
        digitalWrite(pinPos, HIGH);
        digitalWrite(pinNeg, LOW);
    }
    else if (value < 0)
    {
        digitalWrite(pinPos, LOW);
        digitalWrite(pinNeg, HIGH);
    }
    else
    {
        digitalWrite(pinPos, LOW);
        digitalWrite(pinNeg, LOW);
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
bool isOutput = false;
unsigned long lastToggle = 0;
const unsigned long TOGGLE_INTERVAL = 500; 
void setup()
{
    attachPin(TOP_POS);
    attachPin(TOP_NEG);

    attachPin(LFT_POS);
    attachPin(LFT_NEG);
    /*pinMode(TOP_POS, OUTPUT);
    pinMode(TOP_NEG, OUTPUT);
    pinMode(LFT_POS, OUTPUT);
    pinMode(LFT_NEG, OUTPUT);*/

    pinMode(COILS_ENABLE, OUTPUT);
    pinMode(ALPHA_CHANNEL, INPUT);
    digitalWrite(COILS_ENABLE, true);
}

void loop()
{
    /*writeBridgeSimple(LFT_POS, LFT_NEG, 1);
    writeBridgeSimple(TOP_POS, TOP_NEG, -1);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, 0);
    writeBridgeSimple(TOP_POS, TOP_NEG, 0);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, -1);
    writeBridgeSimple(TOP_POS, TOP_NEG, 1);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, 0);
    writeBridgeSimple(TOP_POS, TOP_NEG, 1);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, 1);
    writeBridgeSimple(TOP_POS, TOP_NEG, 0);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, -1);
    writeBridgeSimple(TOP_POS, TOP_NEG, 0);
    delay(500);
    writeBridgeSimple(LFT_POS, LFT_NEG, 0);
    writeBridgeSimple(TOP_POS, TOP_NEG, -1);
    delay(500);*/
    float t = micros() * 1e-6f * speed;

    float x = sinf(t) * amplitude;
    float y = cosf(t) * amplitude;

    setXY(x,y);

    unsigned long now = millis();

    if (now - lastToggle >= TOGGLE_INTERVAL) {
        lastToggle = now;

        isOutput = !isOutput;

        if (isOutput) {
            pinMode(ALPHA_CHANNEL, OUTPUT);
            digitalWrite(ALPHA_CHANNEL, LOW);
        } else {
            pinMode(ALPHA_CHANNEL, INPUT);
        }
    }

    //delayMicroseconds(250);
}