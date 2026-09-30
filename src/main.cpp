#include <Arduino.h>

// ============================================================
// Hardware
// ============================================================

// X deflection driver
constexpr int X_PWM_PIN = 16;
constexpr int X_DIR_PIN = 17;

// Y deflection driver
constexpr int Y_PWM_PIN = 18;
constexpr int Y_DIR_PIN = 19;

// Shared driver sleep
constexpr int DRIVER_SLEEP_PIN = 5;

// Beam intensity / blanking
constexpr int BEAM_ALPHA_PIN = 15;


// ============================================================
// Conservative test configuration
// ============================================================

// Keep this well below the previously observed ~150 limit.
constexpr int DEFLECTION_LIMIT = 140;

// Small scan range.
constexpr int MIN_POSITION = -100;
constexpr int MAX_POSITION = 100;

// Very slow movement.
// Increase/decrease later once everything is stable.
constexpr int STEP_DELAY_MS = 10;

// Pause after each test.
constexpr int TEST_PAUSE_MS = 1000;

// DRV8876 PWM
constexpr int PWM_FREQUENCY = 100000;
constexpr int PWM_RESOLUTION = 8;


// ============================================================
// Axis control
// ============================================================

void setX(int position)
{
    position = constrain(
        position,
        -DEFLECTION_LIMIT,
        DEFLECTION_LIMIT
    );

    if (position > 0)
    {
        digitalWrite(X_DIR_PIN, LOW);
        ledcWrite(X_PWM_PIN, position);
    }
    else if (position < 0)
    {
        digitalWrite(X_DIR_PIN, HIGH);
        ledcWrite(X_PWM_PIN, -position);
    }
    else
    {
        ledcWrite(X_PWM_PIN, 0);
    }
}


void setY(int position)
{
    position = constrain(
        position,
        -DEFLECTION_LIMIT,
        DEFLECTION_LIMIT
    );

    if (position > 0)
    {
        digitalWrite(Y_DIR_PIN, LOW);
        ledcWrite(Y_PWM_PIN, position);
    }
    else if (position < 0)
    {
        digitalWrite(Y_DIR_PIN, HIGH);
        ledcWrite(Y_PWM_PIN, -position);
    }
    else
    {
        ledcWrite(Y_PWM_PIN, 0);
    }
}


// ============================================================
// Beam control
// ============================================================

void beamOn()
{
    //digitalWrite(BEAM_ALPHA_PIN, HIGH);
}


void beamOff()
{
    //digitalWrite(BEAM_ALPHA_PIN, LOW);
}


// ============================================================
// Stop both coils
// ============================================================

void stopDeflection()
{
    setX(0);
    setY(0);
}


// ============================================================
// Initialization
// ============================================================

void setup()
{
    pinMode(X_PWM_PIN, OUTPUT);
    pinMode(X_DIR_PIN, OUTPUT);

    pinMode(Y_PWM_PIN, OUTPUT);
    pinMode(Y_DIR_PIN, OUTPUT);

    pinMode(DRIVER_SLEEP_PIN, OUTPUT);
    pinMode(BEAM_ALPHA_PIN, OUTPUT);

    // Keep drivers asleep while configuring.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    digitalWrite(BEAM_ALPHA_PIN, HIGH);
    //beamOff();

    ledcAttach(
        X_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    ledcAttach(
        Y_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    stopDeflection();

    delay(1000);

    // Wake both drivers.
    digitalWrite(DRIVER_SLEEP_PIN, HIGH);

    delay(500);
}


// ============================================================
// X-axis test
// ============================================================

void testX()
{
    // Make absolutely sure Y is inactive.
    setY(0);

    // Move X slowly from center to one side.
    beamOn();

    for (int x = 0; x <= MAX_POSITION; ++x)
    {
        setX(x);
        delay(STEP_DELAY_MS);
    }

    delay(TEST_PAUSE_MS);

    // Return with beam OFF.
    beamOff();

    for (int x = MAX_POSITION; x >= 0; --x)
    {
        setX(x);
        delay(STEP_DELAY_MS);
    }

    setX(0);

    delay(TEST_PAUSE_MS);


    // Move X to the other side.
    beamOn();

    for (int x = 0; x >= MIN_POSITION; --x)
    {
        setX(x);
        delay(STEP_DELAY_MS);
    }

    delay(TEST_PAUSE_MS);

    // Return with beam OFF.
    beamOff();

    for (int x = MIN_POSITION; x <= 0; ++x)
    {
        setX(x);
        delay(STEP_DELAY_MS);
    }

    setX(0);

    delay(TEST_PAUSE_MS);
}


// ============================================================
// Y-axis test
// ============================================================

void testY()
{
    // Make absolutely sure X is inactive.
    setX(0);

    // Move Y slowly from center to one side.
    beamOn();

    for (int y = 0; y <= MAX_POSITION; ++y)
    {
        setY(y);
        delay(STEP_DELAY_MS);
    }

    delay(TEST_PAUSE_MS);

    // Return with beam OFF.
    beamOff();

    for (int y = MAX_POSITION; y >= 0; --y)
    {
        setY(y);
        delay(STEP_DELAY_MS);
    }

    setY(0);

    delay(TEST_PAUSE_MS);


    // Move Y to the other side.
    beamOn();

    for (int y = 0; y >= MIN_POSITION; --y)
    {
        setY(y);
        delay(STEP_DELAY_MS);
    }

    delay(TEST_PAUSE_MS);

    // Return with beam OFF.
    beamOff();

    for (int y = MIN_POSITION; y <= 0; ++y)
    {
        setY(y);
        delay(STEP_DELAY_MS);
    }

    setY(0);

    delay(TEST_PAUSE_MS);
}


// ============================================================
// Main loop
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // X axis only
    // --------------------------------------------------------

    testX();

    // Everything off for a while.
    beamOff();
    stopDeflection();

    delay(2000);


    // --------------------------------------------------------
    // Y axis only
    // --------------------------------------------------------

    testY();

    // Everything off for a while.
    beamOff();
    stopDeflection();

    delay(3000);
}