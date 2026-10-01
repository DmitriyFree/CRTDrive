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

void showFilledSquare()
{
    const unsigned long startTime = millis();

    while (
        millis() - startTime <
        FIGURE_TIME_MS
    )
    {
        drawFilledSquare(100);
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