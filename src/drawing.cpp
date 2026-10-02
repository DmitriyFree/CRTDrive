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
    constexpr int DIGIT_W = 100;
    constexpr int DIGIT_H = 200;
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
    constexpr int cx = 0, cy = 0;

    const float angles[] = {
        2 * PI * ((hours % 12 + minutes / 60.0f) / 12.0f) - PI / 2,
        2 * PI * ((minutes + seconds / 60.0f) / 60.0f) - PI / 2,
        2 * PI * (seconds / 60.0f) - PI / 2
    };

    const int lengths[] = {150, 200, 240};

    for (int i = 0; i < 3; ++i)
    {
        const int x = cx + lengths[i] * cosf(angles[i]);
        const int y = cy + lengths[i] * sinf(angles[i]);

        drawLine(cx, cy, x, y);
        delayMicroseconds(300);
        drawLine(x, y, cx, cy);
        delayMicroseconds(300); // Small delay to allow the beam to dwell on the line
    }
}

void drawDwellChessRaster(int size)
{
    constexpr int CELLS = 4;

    // Dwell durations in microseconds, darkest to brightest.
    constexpr int DWELL[4] = {
        0, 2, 5, 10
    };

    const int cellSize = size / CELLS;
    const int left = -size / 2;
    const int top = -size / 2;

    for (int y = 0; y < size; ++y)
    {
        const bool reverse = y & 1;

        for (int i = 0; i < size; ++i)
        {
            const int x = reverse ? size - 1 - i : i;

            const int cellX = x / cellSize;
            const int cellY = y / cellSize;

            // Four brightness levels in a repeating pattern.
            const int level = (cellX + cellY) % 4;

            setX(left + x);
            setY(top + y);

            delayMicroseconds(DWELL[level]);
        }
    }
}

// 5x7 font: digits, uppercase letters, space and common punctuation.
struct Glyph
{
    char character;
    uint8_t rows[7];
};

static const Glyph FONT[] = {
    {'A',{14,17,17,31,17,17,17}},
    {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}},
    {'D',{30,17,17,17,17,17,30}},
    {'E',{31,16,16,30,16,16,31}},
    {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,15}},
    {'H',{17,17,17,31,17,17,17}},
    {'I',{14,4,4,4,4,4,14}},
    {'J',{7,2,2,2,2,18,12}},
    {'K',{17,18,20,24,20,18,17}},
    {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}},
    {'N',{17,25,21,19,17,17,17}},
    {'O',{14,17,17,17,17,17,14}},
    {'P',{30,17,17,30,16,16,16}},
    {'Q',{14,17,17,17,21,18,13}},
    {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}},
    {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}},
    {'V',{17,17,17,17,17,10,4}},
    {'W',{17,17,17,21,21,21,10}},
    {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}},
    {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}},
    {'1',{4,12,4,4,4,4,14}},
    {'2',{14,17,1,2,4,8,31}},
    {'3',{30,1,1,14,1,1,30}},
    {'4',{2,6,10,18,31,2,2}},
    {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}},
    {'7',{31,1,2,4,8,8,8}},
    {'8',{14,17,17,14,17,17,14}},
    {'9',{14,17,17,15,1,1,14}},
    {'-',{0,0,0,31,0,0,0}},
    {'.',{0,0,0,0,0,12,12}},
    {':',{0,12,12,0,12,12,0}},
    {'!',{4,4,4,4,4,0,4}},
    {'?',{14,17,1,2,4,0,4}},
    {' ',{0,0,0,0,0,0,0}}
};

static const Glyph* findGlyph(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';

    for (const auto& glyph : FONT)
    {
        if (glyph.character == c)
            return &glyph;
    }

    return nullptr;
}

void drawText(const char* text, int x, int y, int pixelSize)
{
    const int charWidth = 6 * pixelSize;

    while (*text)
    {
        const Glyph* glyph = findGlyph(*text);

        if (glyph)
        {
            for (int row = 0; row < 7; ++row)
            {
                for (int col = 0; col < 5; ++col)
                {
                    if (glyph->rows[row] & (1 << (4 - col)))
                    {
                        const int px = x + col * pixelSize;
                        const int py = y + row * pixelSize;

                        drawLine(
                            px, py,
                            px + pixelSize - 1, py
                        );
                        delayMicroseconds(500);
                    }
                }
            }
        }

        x += charWidth;
        ++text;
    }
}
void drawScrollingText(const char* text, int x, int y, int pixelSize)
{
    constexpr int visibleColumns = 12;
    constexpr int scrollDelayUs = 2500; // 5x slower

    constexpr int charWidth = 6; // 5 glyph columns + 1 spacing

    int textLength = 0;
    while (text[textLength]) ++textLength;

    const int totalColumns = textLength * charWidth;

    if (totalColumns == 0)
        return;

    int offset = 0;

    while (true)
    {
        for (int col = 0; col < visibleColumns; ++col)
        {
            const int textCol = (offset + col) % totalColumns;
            const int charIndex = textCol / charWidth;
            const int glyphCol = textCol % charWidth;

            if (glyphCol >= 5)
                continue;

            const Glyph* glyph = findGlyph(text[charIndex]);
            if (!glyph)
                continue;

            for (int row = 0; row < 7; ++row)
            {
                if (glyph->rows[row] & (1 << (4 - glyphCol)))
                {
                    const int px = x + col * pixelSize;
                    const int py = y + row * pixelSize;

                    drawLine(px, py, px + pixelSize - 1, py);
                    delayMicroseconds(scrollDelayUs);
                }
            }
        }

        offset = (offset + 1) % totalColumns;
    }
}