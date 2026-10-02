#include "drawing.h"
#include "hardware.h"

#include <Arduino.h>
#include <math.h>

// ============================================================
// Configuration
// ============================================================

constexpr unsigned long FIGURE_TIME_MS = 1000;

constexpr int CIRCLE_POINTS = 120;

constexpr int FILL_STEP = 3;


// ============================================================
// Line
// ============================================================

void drawLine(
    int x1,
    int y1,
    int x2,
    int y2
)
{
    const int dx = abs(x2 - x1);
    const int sx = x1 < x2 ? 1 : -1;

    const int dy = -abs(y2 - y1);
    const int sy = y1 < y2 ? 1 : -1;

    int error = dx + dy;

    while (true)
    {
        setPoint(x1, y1);

        if (x1 == x2 && y1 == y2)
            break;

        const int e2 = 2 * error;

        if (e2 >= dy)
        {
            error += dy;
            x1 += sx;
        }

        if (e2 <= dx)
        {
            error += dx;
            y1 += sy;
        }
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
// Filled square
// ============================================================

void drawFilledSquare(int size)
{
    bool leftToRight = true;

    for (
        int y = -size;
        y <= size;
        y += FILL_STEP
    )
    {
        setY(y);

        if (leftToRight)
        {
            for (int x = -size; x <= size; ++x)
            {
                setX(x);
                waitPoint();
            }
        }
        else
        {
            for (int x = size; x >= -size; --x)
            {
                setX(x);
                waitPoint();
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
// Timed figure
// ============================================================

void showFigure(
    void (*figure)(int),
    int size
)
{
    const unsigned long startTime = millis();

    while (
        millis() - startTime <
        FIGURE_TIME_MS
    )
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

    while (
        millis() - startTime <
        FIGURE_TIME_MS
    )
    {
        drawConcentricCircles();
    }

    stopDeflection();
}


// ============================================================
// Timed filled square
// ============================================================

void showFilledSquare(int size)
{
    const unsigned long startTime = millis();

    while (
        millis() - startTime <
        FIGURE_TIME_MS
    )
    {
        drawFilledSquare(size);
    }

    stopDeflection();
}

void drawChessRaster(int size)
{
    const int half = size / 2;
    const int left = -half;
    const int top = -half;

    constexpr int CELLS = 4;
    const int cellSize = size / CELLS;

    for (int y = 0; y < size; ++y)
    {
        const int py = top + y;

        // Serpentine raster
        const bool reverse = (y & 1);

        for (int i = 0; i < size; ++i)
        {
            if(!i%50)continue; // Skip some points to reduce the number of points drawn
            const int x = reverse
                ? (size - 1 - i)
                : i;

            const int px = left + x;

            const int cellX = x / cellSize;
            const int cellY = y / cellSize;

            const bool filled = ((cellX + cellY) & 1) == 0;

            if (filled)
                beamOn();
            else
                beamOff();

            setPoint(px, py);
        }
    }

    beamOff();
}
void showChessRaster(int size)
{
    const unsigned long start = millis();

    while (millis() - start < FIGURE_TIME_MS)
    {
        drawChessRaster(size);
    }

    beamOff();
    stopDeflection();
}

// Seven-segment digit, using a 40 x 60 logical grid.
//
//       A
//     ─────
//  F │     │ B
//    │  G  │
//     ─────
//  E │     │ C
//    │     │
//     ─────
//       D

void drawDigit(int digit, int x, int y, int w, int h)
{
    constexpr int A = 1 << 0;
    constexpr int B = 1 << 1;
    constexpr int C = 1 << 2;
    constexpr int D = 1 << 3;
    constexpr int E = 1 << 4;
    constexpr int F = 1 << 5;
    constexpr int G = 1 << 6;

    constexpr int segments[] = {
        A | B | C | D | E | F,       // 0
        B | C,                       // 1
        A | B | G | E | D,           // 2
        A | B | G | C | D,           // 3
        F | G | B | C,               // 4
        A | F | G | C | D,           // 5
        A | F | G | E | C | D,       // 6
        A | B | C,                   // 7
        A | B | C | D | E | F | G,  // 8
        A | B | C | D | F | G        // 9
    };

    if (digit < 0 || digit > 9)
        return;

    const int x0 = x;
    const int x1 = x + w;
    const int xm = x + w / 2;
    const int y0 = y;
    const int ym = y + h / 2;
    const int y1 = y + h;

    const int s = segments[digit];

    if (s & A) drawLine(x0, y0, x1, y0);
    if (s & B) drawLine(x1, y0, x1, ym);
    if (s & C) drawLine(x1, ym, x1, y1);
    if (s & D) drawLine(x0, y1, x1, y1);
    if (s & E) drawLine(x0, ym, x0, y1);
    if (s & F) drawLine(x0, y0, x0, ym);
    if (s & G) drawLine(x0, ym, x1, ym);
}
void drawClock(int hours, int minutes, int seconds)
{
    constexpr int DIGIT_W = 24;
    constexpr int DIGIT_H = 48;
    constexpr int GAP = 5;
    constexpr int COLON_W = 10;

    constexpr int TOTAL_W =
        6 * DIGIT_W + 5 * GAP + 2 * COLON_W;

    const int left = -TOTAL_W / 2;
    const int top = -DIGIT_H / 2;

    const int digits[] = {
        hours / 10, hours % 10,
        minutes / 10, minutes % 10,
        seconds / 10, seconds % 10
    };

    int x = left;

    for (int i = 0; i < 6; ++i)
    {
        drawDigit(digits[i], x, top, DIGIT_W, DIGIT_H);
        x += DIGIT_W + GAP;

        if (i == 1 || i == 3)
        {
            // Colon represented by two tiny horizontal strokes.
            const int cx = x + COLON_W / 2;
            const int cy1 = top + DIGIT_H / 3;
            const int cy2 = top + 2 * DIGIT_H / 3;

            drawLine(cx - 1, cy1, cx + 1, cy1);
            drawLine(cx - 1, cy2, cx + 1, cy2);

            x += COLON_W;
        }
    }
}
void drawAnalogClock(int hours, int minutes, int seconds)
{
    //constexpr float PI = 3.14159265f;
    constexpr int RADIUS = 75;

    const int cx = 0;
    const int cy = 0;

    // Draw the outer dial as a polygon.
    constexpr int CIRCLE_POINTS = 120;

    int prevX = cx + RADIUS;
    int prevY = cy;

    for (int i = 1; i <= CIRCLE_POINTS; ++i)
    {
        const float angle = 2.0f * PI * i / CIRCLE_POINTS;
        const int x = cx + (int)(RADIUS * cosf(angle));
        const int y = cy + (int)(RADIUS * sinf(angle));

        drawLine(prevX, prevY, x, y);

        prevX = x;
        prevY = y;
    }

    // Hour and minute tick marks.
    for (int i = 0; i < 60; ++i)
    {
        const float angle = 2.0f * PI * i / 60.0f - PI / 2.0f;

        const bool major = (i % 5 == 0);
        const int outer = RADIUS - 2;
        const int inner = major ? RADIUS - 12 : RADIUS - 5;

        const int x1 = cx + (int)(outer * cosf(angle));
        const int y1 = cy + (int)(outer * sinf(angle));
        const int x2 = cx + (int)(inner * cosf(angle));
        const int y2 = cy + (int)(inner * sinf(angle));

        drawLine(x1, y1, x2, y2);
    }

    // Convert time to angles.
    const float secondAngle =
        2.0f * PI * (seconds / 60.0f) - PI / 2.0f;

    const float minuteAngle =
        2.0f * PI * ((minutes + seconds / 60.0f) / 60.0f)
        - PI / 2.0f;

    const float hourAngle =
        2.0f * PI * ((hours % 12 + minutes / 60.0f) / 12.0f)
        - PI / 2.0f;

    // Draw hands from the center.
    const int hourLength = 38;
    const int minuteLength = 58;
    const int secondLength = 66;

    drawLine(cx, cy,
             cx + (int)(hourLength * cosf(hourAngle)),
             cy + (int)(hourLength * sinf(hourAngle)));

    drawLine(cx, cy,
             cx + (int)(minuteLength * cosf(minuteAngle)),
             cy + (int)(minuteLength * sinf(minuteAngle)));

    drawLine(cx, cy,
             cx + (int)(secondLength * cosf(secondAngle)),
             cy + (int)(secondLength * sinf(secondAngle)));
}