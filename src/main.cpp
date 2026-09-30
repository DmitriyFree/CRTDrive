#include <Arduino.h>
#include "soc/gpio_struct.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"

inline void fastWrite(int pin, bool level)
{
    if (level)
        GPIO.out_w1ts = (1UL << pin);
    else
        GPIO.out_w1tc = (1UL << pin);
}

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

// Shape size
constexpr int SHAPE_SIZE = 100;

// Keep each figure on screen for this long.
constexpr unsigned long FIGURE_TIME_MS = 1000;

// Circle resolution
constexpr int CIRCLE_POINTS = 120;

// Filled square raster spacing.
// Smaller = denser fill, but more work.
constexpr int FILL_STEP = 3;

// DRV8876 PWM
constexpr int PWM_FREQUENCY = 100000;
constexpr int PWM_RESOLUTION = 8;


// ============================================================
// Remember direction through zero
// ============================================================

int lastXDirection = 1;
int lastYDirection = 1;

// ============================================================
// Fast LEDC
// ============================================================

inline void fastPwmX(int duty)
{
    LEDC.channel_group[0].channel[0].duty.duty =
        ((uint32_t)duty << 4);

    LEDC.channel_group[0].channel[0].conf1.duty_start = 1;
}

inline void fastPwmY(int duty)
{
    LEDC.channel_group[0].channel[1].duty.duty =
        ((uint32_t)duty << 4);

    LEDC.channel_group[0].channel[1].conf1.duty_start = 1;
}

// ============================================================
// Axis control
// ============================================================
int currentXDirection = 1;
int currentYDirection = 1;

inline void setXFast(int x)
{
    if (x > 0)
    {
        if (currentXDirection != 1)
        {
            fastWrite(X_DIR_PIN, LOW);
            currentXDirection = 1;
        }

        fastPwmX(x);
    }
    else if (x < 0)
    {
        if (currentXDirection != -1)
        {
            fastWrite(X_DIR_PIN, HIGH);
            currentXDirection = -1;
        }

        fastPwmX(-x);
    }
    else
    {
        fastPwmX(1);
    }
}

inline void setYFast(int y)
{
    y = -y;

    if (y > 0)
    {
        if (currentYDirection != 1)
        {
            fastWrite(Y_DIR_PIN, LOW);
            currentYDirection = 1;
        }

        fastPwmY(y);
    }
    else if (y < 0)
    {
        if (currentYDirection != -1)
        {
            fastWrite(Y_DIR_PIN, HIGH);
            currentYDirection = -1;
        }

        fastPwmY(-y);
    }
    else
    {
        fastPwmY(1);
    }
}
void setX(int position)
{
    if (position > 0)
    {
        lastXDirection = 1;

        fastWrite(X_DIR_PIN, LOW);
        ledcWrite(X_PWM_PIN, position);
    }
    else if (position < 0)
    {
        lastXDirection = -1;

        fastWrite(X_DIR_PIN, HIGH);
        ledcWrite(X_PWM_PIN, -position);
    }
    else
    {
        fastWrite(
            X_DIR_PIN,
            lastXDirection > 0 ? LOW : HIGH
        );

        ledcWrite(X_PWM_PIN, 1);
    }
}

void setY(int position)
{
    position = -position;

    if (position > 0)
    {
        lastYDirection = 1;

        fastWrite(Y_DIR_PIN, LOW);
        ledcWrite(Y_PWM_PIN, position);
    }
    else if (position < 0)
    {
        lastYDirection = -1;

        fastWrite(Y_DIR_PIN, HIGH);
        ledcWrite(Y_PWM_PIN, -position);
    }
    else
    {
        fastWrite(
            Y_DIR_PIN,
            lastYDirection > 0 ? LOW : HIGH
        );

        ledcWrite(Y_PWM_PIN, 1);
    }
}
inline void setPoint(int x, int y)
{
    // --------------------------------------------------------
    // X
    // --------------------------------------------------------

    if (x > 0)
    {
        fastWrite(X_DIR_PIN, LOW);
        fastPwmX(x);
    }
    else if (x < 0)
    {
        fastWrite(X_DIR_PIN, HIGH);
        fastPwmX(-x);
    }
    else
    {
        fastPwmX(1);
    }


    // --------------------------------------------------------
    // Y
    // --------------------------------------------------------

    y = -y;

    if (y > 0)
    {
        fastWrite(Y_DIR_PIN, LOW);
        fastPwmY(y);
    }
    else if (y < 0)
    {
        fastWrite(Y_DIR_PIN, HIGH);
        fastPwmY(-y);
    }
    else
    {
        fastPwmY(1);
    }
}

// ============================================================
// Beam control
// ============================================================

// Alpha intentionally ignored.
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
    // For an actual stop, unlike a drawing zero-crossing,
    // we really want zero current.

    ledcWrite(X_PWM_PIN, 0);
    ledcWrite(Y_PWM_PIN, 0);
}

// ============================================================
// Fast integer line drawing
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
        setPoint(x1, y1);
        return;
    }

    for (int i = 0; i <= steps; ++i)
    {
        const int x =
            x1 + ((x2 - x1) * i) / steps;

        const int y =
            y1 + ((y2 - y1) * i) / steps;

        setPoint(x, y);
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
    drawLine(
        -size, -size,
         size, -size
    );

    drawLine(
         size, -size,
         0, size
    );

    drawLine(
         0, size,
        -size, -size
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
                radius * cosf(angle)
            );

        const int y =
            static_cast<int>(
                radius * sinf(angle)
            );

        setPoint(x, y);
    }
}


// ============================================================
// Fast filled square
// ============================================================

void drawFilledSquare(int size)
{
    bool leftToRight = true;

    for (int y = -size;
         y <= size;
         y += FILL_STEP)
    {
        setYFast(y);

        if (leftToRight)
        {
            // X is already negative at the beginning.
            if (currentXDirection != -1)
            {
                fastWrite(X_DIR_PIN, HIGH);
                currentXDirection = -1;
            }

            for (int x = -size; x <= size; ++x)
            {
                if (x == 0)
                    fastPwmX(1);
                else
                    fastPwmX(abs(x));
            }
        }
        else
        {
            // X is already positive at the beginning.
            if (currentXDirection != 1)
            {
                fastWrite(X_DIR_PIN, LOW);
                currentXDirection = 1;
            }

            for (int x = size; x >= -size; --x)
            {
                if (x == 0)
                    fastPwmX(1);
                else
                    fastPwmX(abs(x));
            }
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
         size, size
    );

    drawLine(
        -size, size,
         size, -size
    );
}


// ============================================================
// Diamond
// ============================================================

void drawDiamond(int size)
{
    drawLine(
        0, size,
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
// Timed vector figure
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
// Timed concentric circles
// ============================================================

void showConcentricCircles()
{
    const unsigned long startTime = millis();

    while (millis() - startTime < FIGURE_TIME_MS)
    {
        drawConcentricCircles();
    }

    stopDeflection();
}


// ============================================================
// Timed filled square
// ============================================================

void showFilledSquare()
{
    const unsigned long startTime = millis();

    while (millis() - startTime < FIGURE_TIME_MS)
    {
        drawFilledSquare(SHAPE_SIZE);
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

    // Keep drivers asleep during configuration.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    // Alpha permanently enabled.
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

    showFigure(
        drawSquare,
        SHAPE_SIZE
    );


    // --------------------------------------------------------
    // Triangle
    // --------------------------------------------------------

    showFigure(
        drawTriangle,
        SHAPE_SIZE
    );


    // --------------------------------------------------------
    // Circle
    // --------------------------------------------------------

    showFigure(
        drawCircle,
        SHAPE_SIZE
    );


    // --------------------------------------------------------
    // Concentric circles
    // --------------------------------------------------------

    showConcentricCircles();


    // --------------------------------------------------------
    // Filled square
    // --------------------------------------------------------

    showFilledSquare();


    // --------------------------------------------------------
    // Cross
    // --------------------------------------------------------

    showFigure(
        drawCross,
        SHAPE_SIZE
    );


    // --------------------------------------------------------
    // Diagonal cross
    // --------------------------------------------------------

    showFigure(
        drawDiagonalCross,
        SHAPE_SIZE
    );


    // --------------------------------------------------------
    // Diamond
    // --------------------------------------------------------

    showFigure(
        drawDiamond,
        SHAPE_SIZE
    );
}