#include <Arduino.h>

// ============================================================
// Hardware
// ============================================================

constexpr int DEFLECTION_PWM_PIN = 16;  // DRV8876 EN / PWM
constexpr int DEFLECTION_DIR_PIN = 17;  // DRV8876 PH / direction
constexpr int DRIVER_SLEEP_PIN   = 5;
constexpr int DRIVER_FAULT_PIN   = 18;

constexpr int BEAM_ALPHA_PIN     = 15;

// ============================================================
// Deflection configuration
// ============================================================

// Experimentally determined stable maximum.
// 150 was unstable, so keep some margin.
constexpr int DEFLECTION_LIMIT = 140;

// Horizontal scan range.
constexpr int MIN_POSITION = -150;
constexpr int MAX_POSITION = 150;

// Number of visible/blank sections.
constexpr int DASH_COUNT = 10;

// 0 = run as fast as possible.
constexpr int STEP_DELAY_US = 0;

// DRV8876 PWM
constexpr int PWM_FREQUENCY = 100000;
constexpr int PWM_RESOLUTION = 8;


// ============================================================
// Deflection control
// ============================================================

void setDeflection(int position)
{
    position = constrain(
        position,
        -DEFLECTION_LIMIT,
        DEFLECTION_LIMIT
    );

    if (position > 0)
    {
        digitalWrite(DEFLECTION_DIR_PIN, LOW);
        ledcWrite(DEFLECTION_PWM_PIN, position);
    }
    else if (position < 0)
    {
        digitalWrite(DEFLECTION_DIR_PIN, HIGH);
        ledcWrite(DEFLECTION_PWM_PIN, -position);
    }
    else
    {
        ledcWrite(DEFLECTION_PWM_PIN, 0);
    }
}


// ============================================================
// Beam control
// ============================================================

void beamOn()
{
    digitalWrite(BEAM_ALPHA_PIN, HIGH);
}

void beamOff()
{
    digitalWrite(BEAM_ALPHA_PIN, LOW);
}


// ============================================================
// Setup
// ============================================================

void setup()
{
    pinMode(DEFLECTION_PWM_PIN, OUTPUT);
    pinMode(DEFLECTION_DIR_PIN, OUTPUT);
    pinMode(DRIVER_SLEEP_PIN, OUTPUT);
    pinMode(DRIVER_FAULT_PIN, INPUT_PULLUP);

    pinMode(BEAM_ALPHA_PIN, OUTPUT);

    // Keep driver asleep while configuring it.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    setDeflection(0);
    beamOff();

    Serial.begin(115200);

    delay(500);

    // Wake driver.
    digitalWrite(DRIVER_SLEEP_PIN, HIGH);
    delay(100);

    ledcAttach(
        DEFLECTION_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    Serial.print("DRV8876 nFAULT = ");
    Serial.println(digitalRead(DRIVER_FAULT_PIN));
}


// ============================================================
// Drawing
// ============================================================

void drawDashes()
{
    for (int position = MIN_POSITION;
         position <= MAX_POSITION;
         ++position)
    {
        const int dash =
            ((position - MIN_POSITION) * DASH_COUNT) /
            (MAX_POSITION - MIN_POSITION + 1);

        if ((dash & 1) == 0)
            beamOn();
        else
            beamOff();

        setDeflection(position);

        delayMicroseconds(STEP_DELAY_US);
    }
}


// ============================================================
// Main loop
// ============================================================

void loop()
{
    // Draw left → right.
    drawDashes();

    // Blank during retrace.
    beamOff();

    // Return right → left.
    for (int position = MAX_POSITION;
         position >= MIN_POSITION;
         --position)
    {
        setDeflection(position);
        delayMicroseconds(STEP_DELAY_US);
    }
}