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

constexpr int DEFLECTION_LIMIT = 140;

// Shape size
constexpr int SHAPE_SIZE = 80;

// Keep each figure on screen for this long.
constexpr unsigned long FIGURE_TIME_MS = 1000;

// Circle resolution
constexpr int CIRCLE_POINTS = 120;

// Filled square spacing
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
    // Y axis is physically inverted.
    position = -position;

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

// Alpha intentionally ignored for now.
// Beam remains ON continuously.

void beamOn()
{
}


void beamOff()
{
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
// Move to a point
// ============================================================

void moveTo(int x, int y)
{
    setX(x);
    setY(y);
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
    drawLine(-size, -size, size, -size);
    drawLine( size, -size, size,  size);
    drawLine( size,  size, -size, size);
    drawLine(-size,  size, -size, -size);
}


// ============================================================
// Triangle
// ============================================================

void drawTriangle(int size)
{
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
    drawLine(-size, 0, size, 0);
    drawLine(0, -size, 0, size);
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
    drawLine(0, size, size, 0);
    drawLine(size, 0, 0, -size);
    drawLine(0, -size, -size, 0);
    drawLine(-size, 0, 0, size);
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
// Keep drawing a figure for a fixed time
// ============================================================

void showFigure(
    void (*figure)(int),
    int size
)
{
    const unsigned long startTime = millis();

    while (millis() - startTime < FIGURE_TIME_MS)
    {
        figure(size);
    }

    stopDeflection();
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

    // Keep drivers asleep while configuring.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    // Alpha is permanently enabled.
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
// Main loop
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Square
    // --------------------------------------------------------

    showFigure(drawSquare, SHAPE_SIZE);


    // --------------------------------------------------------
    // Triangle
    // --------------------------------------------------------

    showFigure(drawTriangle, SHAPE_SIZE);


    // --------------------------------------------------------
    // Circle
    // --------------------------------------------------------

    showFigure(drawCircle, SHAPE_SIZE);


    // --------------------------------------------------------
    // Concentric circles
    // --------------------------------------------------------

    const unsigned long circlesStart = millis();

    while (millis() - circlesStart < FIGURE_TIME_MS)
    {
        drawConcentricCircles();
    }

    stopDeflection();


    // --------------------------------------------------------
    // Filled square
    // --------------------------------------------------------

    const unsigned long filledStart = millis();

    while (millis() - filledStart < FIGURE_TIME_MS)
    {
        drawFilledSquare(SHAPE_SIZE);
    }

    stopDeflection();


    // --------------------------------------------------------
    // Cross
    // --------------------------------------------------------

    showFigure(drawCross, SHAPE_SIZE);


    // --------------------------------------------------------
    // Diagonal cross
    // --------------------------------------------------------

    showFigure(drawDiagonalCross, SHAPE_SIZE);


    // --------------------------------------------------------
    // Diamond
    // --------------------------------------------------------

    showFigure(drawDiamond, SHAPE_SIZE);


    // Short pause before repeating.
    delay(500);
}