#include "drawing.h"
#include "hardware.h"
#include "font.h"
#include "common.h"

#include <Arduino.h>
#include <math.h>
#include <pgmspace.h>

void drawLine(
    int x1,
    int y1,
    int x2,
    int y2,
    int delayUs = 0
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
        delayMicroseconds(delayUs);

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
void drawSquare(int size, int delayUs)
{
    drawLine(-size, -size, size, -size, delayUs);
    drawLine( size, -size, size,  size, delayUs);
    drawLine( size,  size, -size, size, delayUs);
    drawLine(-size,  size, -size, -size, delayUs);
}
void drawTriangle(int size, int delayUs)
{
    drawLine(-size, -size, size, -size, delayUs );
    drawLine(size, -size, 0, size, delayUs);
    drawLine(0, size, -size, -size, delayUs);
}
void drawCircle(int radius, int delayUs)
{
    constexpr int CIRCLE_POINTS = 120;
    for (int i = 0; i <= CIRCLE_POINTS; ++i)
    {
        const float angle = (2.0f * PI * i) / CIRCLE_POINTS;
        const int x = static_cast<int>( radius * cosf(angle) );
        const int y = static_cast<int>( radius * sinf(angle) );
        setPoint(x, y);
        delayMicroseconds(delayUs);
    }
}
void drawFilledSquare(int size)
{
    //todo raster square in the middle 1/2 screen size
}
void drawCross(int size, int delayUs)
{
    drawLine(-size, 0, size, 0, delayUs);
    drawLine(0, -size, 0, size, delayUs);
}
void drawDiagonalCross(int size, int delayUs)
{
    drawLine( -size, -size, size, size, delayUs);
    drawLine( -size, size, size, -size, delayUs);
}
void drawDiamond(int size, int delayUs)
{
    drawLine(0, size, size, 0, delayUs);
    drawLine(size, 0,  0, -size, delayUs);
    drawLine(0, -size, -size, 0, delayUs);
    drawLine(-size, 0, 0, size, delayUs);
}
void drawConcentricCircles(int delayUs)
{
    drawCircle(25, delayUs);
    drawCircle(50, delayUs);
    drawCircle(75, delayUs);
}
void drawClock(int hours, int minutes, int seconds)
{
    //todo, use draw text
}
void drawAnalogClock(int hours, int minutes, int seconds, int delayUs)
{
    constexpr int cx = 0, cy = 0;
    float pi = 3.141592;
    const float angles[] = {
        2 *pi * ((hours % 12 + minutes / 60.0f) / 12.0f) - pi / 2,
        2 * pi * ((minutes + seconds / 60.0f) / 60.0f) - pi / 2,
        2 * pi * (seconds / 60.0f) - pi / 2
    };

    const int lengths[] = {200, 250, 400};

    for (int i = 0; i < 3; ++i)
    {
        const int x = cx + lengths[i] * cosf(angles[i]);
        const int y = cy + lengths[i] * sinf(angles[i]);

        drawLine(cx, cy, x, y, delayUs);
        drawLine(x, y, cx, cy, delayUs);
    }
}

void drawChess(int cells)
{
    //TODO: fill unpackedFrame frame with chess pattern and run video
}

void drawText(const char* text, int x, int y, int pixelSize, int delayUs)
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
                        
                    }
                    delayMicroseconds(delayUs);
                }
            }
        }

        x += charWidth;
        ++text;
    }
}
void drawScrollingText(const char* text, int x, int y, int pixelSize)
{
    constexpr int visibleColumns = 24;
    constexpr int glyphBudgetUs = 15000;
    constexpr int charWidth = 6;
    constexpr int maxCharacters = 128;

    // Decode UTF-8 string into Unicode codepoints
    uint32_t characters[maxCharacters];
    int textLength = 0;

    const char* ptr = text;

    while (*ptr && textLength < maxCharacters)
    {
        characters[textLength++] = decodeUTF8(ptr);
    }

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

            // Has this glyph already been processed this frame?
            bool alreadyProcessed = false;

            for (int prev = 0; prev < col; ++prev)
            {
                const int prevTextCol = (offset + prev) % totalColumns;

                if (prevTextCol / charWidth == charIndex)
                {
                    alreadyProcessed = true;
                    break;
                }
            }

            if (alreadyProcessed)
                continue;

            const Glyph* glyph = findGlyph(characters[charIndex]);

            // Count all lines in this glyph
            int lineCount = 0;

            if (glyph)
            {
                for (int row = 0; row < 7; ++row)
                {
                    for (int c = 0; c < 5; ++c)
                    {
                        if (glyph->rows[row] & (1 << (4 - c)))
                            ++lineCount;
                    }
                }
            }

            // Space / unknown glyph
            if (lineCount == 0)
            {
                delayMicroseconds(glyphBudgetUs);
                continue;
            }

            // Equal total time for every glyph
            const int delayUs = glyphBudgetUs / lineCount;

            // Draw visible portion of glyph
            for (int row = 0; row < 7; ++row)
            {
                for (int c = 0; c < 5; ++c)
                {
                    if (!(glyph->rows[row] & (1 << (4 - c))))
                        continue;

                    const int glyphTextCol = charIndex * charWidth + c;

                    const int relativeCol =
                        (glyphTextCol - offset + totalColumns)
                        % totalColumns;

                    // Outside visible window
                    if (relativeCol >= visibleColumns)
                        continue;

                    const int px = x + relativeCol * pixelSize;
                    const int py = y + row * pixelSize;
                    beamOn();
                    drawLine(
                        px,
                        py,
                        px + pixelSize - 1,
                        py,
                        5
                    );
                    delayMicroseconds(delayUs);
                    beamOff();
                }
            }
        }

        offset = (offset + 1) % totalColumns;
    }
}
void drawRotatingCube(int cx, int cy, int size, int delayUs)
{
    static float angleX = 0;
    static float angleY = 0;

    // 8 vertices of a cube, centered at origin
    const float vertices[8][3] = {
        {-1, -1, -1},
        { 1, -1, -1},
        { 1,  1, -1},
        {-1,  1, -1},
        {-1, -1,  1},
        { 1, -1,  1},
        { 1,  1,  1},
        {-1,  1,  1}
    };

    // 12 edges connecting the vertices
    const int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    float projected[8][2];

    const float cosX = cosf(angleX);
    const float sinX = sinf(angleX);
    const float cosY = cosf(angleY);
    const float sinY = sinf(angleY);

    for (int i = 0; i < 8; ++i)
    {
        float x = vertices[i][0];
        float y = vertices[i][1];
        float z = vertices[i][2];

        // Rotate around X
        float y1 = y * cosX - z * sinX;
        float z1 = y * sinX + z * cosX;

        // Rotate around Y
        float x2 = x * cosY + z1 * sinY;
        float z2 = -x * sinY + z1 * cosY;

        // Perspective projection
        const float cameraDistance = 4.0f;
        const float perspective = 1.0f / (cameraDistance + z2);

        projected[i][0] = cx + x2 * size * perspective;
        projected[i][1] = cy + y1 * size * perspective;
    }

    // Draw all edges
    for (int i = 0; i < 12; ++i)
    {
        int a = edges[i][0];
        int b = edges[i][1];

        drawLine(
            (int)projected[a][0],
            (int)projected[a][1],
            (int)projected[b][0],
            (int)projected[b][1],
            delayUs
        );
    }

    // Advance rotation
    angleX += 0.025f;
    angleY += 0.018f;

    if (angleX >= 2 * PI) angleX -= 2 * PI;
    if (angleY >= 2 * PI) angleY -= 2 * PI;
}