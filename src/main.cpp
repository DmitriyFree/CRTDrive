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
// Configuration
// ============================================================

// Absolute hardware limit.
// Previously observed stable maximum was about 140.
constexpr int DEFLECTION_LIMIT = 140;

// Shape size.
// Keep this below the full driver range for now.
constexpr int SHAPE_SIZE = 80;

// Time between individual points.
constexpr int STEP_DELAY_MS = 0;

// Pause after each shape.
constexpr int TEST_PAUSE_MS = 0;

// Number of points used for circles.
constexpr int CIRCLE_POINTS = 120;

// Number of horizontal lines in filled square.
constexpr int FILL_STEP = 4;

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

// Alpha is intentionally NOT used for now.
// Beam stays on continuously.

void beamOn()
{
    // Intentionally empty.
}


void beamOff()
{
    // Intentionally empty.
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
// Move to a 2D point
// ============================================================

void moveTo(int x, int y)
{
    setX(x);
    setY(y);

    delay(STEP_DELAY_MS);
}


// ============================================================
// Draw a line
// ============================================================

void drawLine(
    int x1,
    int y1,
    int x2,
    int y2
)
{
    const int dx = abs(x2 - x1);
    const int dy = abs(y2 - y1);

    const int steps = max(dx, dy);

    if (steps == 0)
    {
        moveTo(x1, y1);
        return;
    }

    for (int i = 0; i <= steps; ++i)
    {
        const float t =
            static_cast<float>(i) /
            static_cast<float>(steps);

        const int x =
            x1 + static_cast<int>(
                (x2 - x1) * t
            );

        const int y =
            y1 + static_cast<int>(
                (y2 - y1) * t
            );

        moveTo(x, y);
    }
}


// ============================================================
// Square
// ============================================================

void drawSquare(int size)
{
    drawLine(
        -size, -size,
         size, -size
    );

    drawLine(
         size, -size,
         size,  size
    );

    drawLine(
         size,  size,
        -size,  size
    );

    drawLine(
        -size,  size,
        -size, -size
    );
}


// ============================================================
// Triangle
// ============================================================

void drawTriangle(int size)
{
    // Equilateral-ish triangle.
    const int topY = size;
    const int bottomY = -size;

    const int leftX = -size;
    const int rightX = size;

    drawLine(
        leftX, bottomY,
        rightX, bottomY
    );

    drawLine(
        rightX, bottomY,
        0, topY
    );

    drawLine(
        0, topY,
        leftX, bottomY
    );
}


// ============================================================
// Circle
// ============================================================

void drawCircle(int radius)
{
    for (int i = 0; i <= CIRCLE_POINTS; ++i)
    {
        const float angle =
            (2.0f * PI * i) /
            CIRCLE_POINTS;

        const int x =
            static_cast<int>(
                radius * cos(angle)
            );

        const int y =
            static_cast<int>(
                radius * sin(angle)
            );

        moveTo(x, y);
    }
}


// ============================================================
// Filled square
// ============================================================

void drawFilledSquare(int size)
{
    bool leftToRight = true;

    for (int y = -size; y <= size; y += FILL_STEP)
    {
        if (leftToRight)
        {
            drawLine(
                -size, y,
                 size, y
            );
        }
        else
        {
            drawLine(
                 size, y,
                -size, y
            );
        }

        leftToRight = !leftToRight;
    }
}


// ============================================================
// Cross
// ============================================================

void drawCross(int size)
{
    drawLine(
        -size, 0,
         size, 0
    );

    drawLine(
        0, -size,
        0,  size
    );
}


// ============================================================
// Diagonal cross
// ============================================================

void drawDiagonalCross(int size)
{
    drawLine(
        -size, -size,
         size,  size
    );

    drawLine(
        -size,  size,
         size, -size
    );
}


// ============================================================
// Diamond
// ============================================================

void drawDiamond(int size)
{
    drawLine(
        0,  size,
        size, 0
    );

    drawLine(
        size, 0,
        0, -size
    );

    drawLine(
        0, -size,
        -size, 0
    );

    drawLine(
        -size, 0,
        0, size
    );
}


// ============================================================
// Concentric circles
// ============================================================

void drawConcentricCircles()
{
    drawCircle(25);
    drawCircle(50);
    drawCircle(75);
}


// ============================================================
// Setup
// ============================================================

void setup()
{
    pinMode(X_PWM_PIN, OUTPUT);
    pinMode(X_DIR_PIN, OUTPUT);

    pinMode(Y_PWM_PIN, OUTPUT);
    pinMode(Y_DIR_PIN, OUTPUT);

    pinMode(DRIVER_SLEEP_PIN, OUTPUT);
    pinMode(BEAM_ALPHA_PIN, OUTPUT);

    // Keep drivers asleep during configuration.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    // IMPORTANT:
    // Alpha is permanently enabled for these tests.
    digitalWrite(BEAM_ALPHA_PIN, HIGH);

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
// Main test sequence
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // 1. Square
    // --------------------------------------------------------

    drawSquare(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 2. Triangle
    // --------------------------------------------------------

    drawTriangle(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 3. Circle
    // --------------------------------------------------------

    drawCircle(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 4. Concentric circles
    // --------------------------------------------------------

    drawConcentricCircles();

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 5. Filled square
    // --------------------------------------------------------

    drawFilledSquare(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 6. Horizontal + vertical cross
    // --------------------------------------------------------

    drawCross(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 7. Diagonal cross
    // --------------------------------------------------------

    drawDiagonalCross(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);


    // --------------------------------------------------------
    // 8. Diamond
    // --------------------------------------------------------

    drawDiamond(SHAPE_SIZE);

    stopDeflection();
    delay(TEST_PAUSE_MS);
}