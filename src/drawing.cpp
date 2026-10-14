#include "drawing.h"
#include "hardware.h"
#include "font.h"
#include "common.h"
#include "VideoDriver.h"

#include <Arduino.h>
#include <math.h>
#include <pgmspace.h>

void drawLine(int x1, int y1, int x2, int y2, int delayUs)
{
    const int dzX = getDisplayDeadZoneX();
    const int dzY = getDisplayDeadZoneY();

    // Lines lying exactly on an axis are moved to its positive side.
    const bool horizontalAxisLine = (y1 == 0 && y2 == 0);
    const bool verticalAxisLine   = (x1 == 0 && x2 == 0);

    auto pushX = [dzX](int x) -> int
    {
        if (x < 0) return x - dzX;
        return x + dzX; // Zero goes to the positive side.
    };

    auto pushY = [dzY](int y) -> int
    {
        if (y < 0) return y - dzY;
        return y + dzY; // Zero goes to the positive side.
    };

    const int dx = abs(x2 - x1);
    const int sx = x1 < x2 ? 1 : -1;

    const int dy = -abs(y2 - y1);
    const int sy = y1 < y2 ? 1 : -1;

    int error = dx + dy;

    while (true)
    {
        int outX = x1;
        int outY = y1;

        if (dzX > 0)
        {
            if (verticalAxisLine)
                outX = dzX;
            else if (x1 != 0)
                outX = pushX(x1);
        }

        if (dzY > 0)
        {
            if (horizontalAxisLine)
                outY = dzY;
            else if (y1 != 0)
                outY = pushY(y1);
        }

        setPoint(outX, outY);

        if (delayUs > 0)
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
        if(abs(x)>getDisplayDeadZoneX() && abs(y)>getDisplayDeadZoneY()){
            setPoint(x, y);
            delayMicroseconds(delayUs);
        }
    }
}
void drawFilledSquare(int size)
{
    size = abs(size);
    if (size == 0)
        return;

    for (int y = -size; y <= size; ++y)
    {
        if ((y + size) & 1)
            drawLine(size, y, -size, y);
        else
            drawLine(-size, y, size, y);
    }
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

void drawClock(int hours, int minutes, int seconds, int delayUs)
{
    char text[9];

    hours %= 24;
    minutes %= 60;
    seconds %= 60;

    if (hours < 0) hours += 24;
    if (minutes < 0) minutes += 60;
    if (seconds < 0) seconds += 60;

    snprintf(
        text, sizeof(text),
        "%02d:%02d:%02d",
        hours, minutes, seconds
    );

    drawText(text, -210, -40, 9, 20, 5*delayUs);
}
void drawAnalogClock(
    int hours,
    int minutes,
    int seconds,
    int delayUs
)
{
    int hourMarks = 12;

    constexpr int cx = 0;
    constexpr int cy = 0;

    hours   = ((hours % 12) + 12) % 12;
    minutes = ((minutes % 60) + 60) % 60;
    seconds = ((seconds % 60) + 60) % 60;

    // Optional hour marks: 0, 4, or 12.
    if (hourMarks == 4 || hourMarks == 12)
    {
        constexpr float markInner = 170.0f;
        constexpr float markOuter = 185.0f;

        for (int i = 0; i < hourMarks+1; ++i)
        {
            const float angle =
                2.0f * PI * i / 12.0f - PI / 2.0f;

            const int x1 = cx + lroundf(markInner * cosf(angle));
            const int y1 = cy + lroundf(markInner * sinf(angle));
            const int x2 = cx + lroundf(markOuter * cosf(angle));
            const int y2 = cy + lroundf(markOuter * sinf(angle));

            drawLine(x1, y1, x2, y2, delayUs);
        }
    }

    const float angles[] = {
        2.0f * PI * (hours + minutes / 60.0f) / 12.0f - PI / 2.0f,
        2.0f * PI * (minutes + seconds / 60.0f) / 60.0f - PI / 2.0f,
        2.0f * PI * seconds / 60.0f - PI / 2.0f
    };

    const int lengths[] = {100, 150, 160};
    const int widths[]  = {20, 12, 1};

    for (int i = 0; i < 3; ++i)
    {
        const float dx = cosf(angles[i]);
        const float dy = sinf(angles[i]);

        const float px = -dy;
        const float py =  dx;

        const float base = (i == 0) ? 12.0f : 5.0f;
        const float halfWidth = widths[i] * 0.5f;

        const int tipX = cx + lroundf(lengths[i] * dx);
        const int tipY = cy + lroundf(lengths[i] * dy);

        const int leftX = cx + lroundf(base * dx + halfWidth * px);
        const int leftY = cy + lroundf(base * dy + halfWidth * py);

        const int rightX = cx + lroundf(base * dx - halfWidth * px);
        const int rightY = cy + lroundf(base * dy - halfWidth * py);

        drawLine(leftX, leftY, rightX, rightY, delayUs);
        drawLine(rightX, rightY, tipX, tipY, delayUs);
        drawLine(tipX, tipY, leftX, leftY, delayUs);
    }
}

void drawChess(int cells)
{
    cells = max(cells, 1);

    for (int y = 0; y < VIDEO_HEIGHT; ++y)
    {
        for (int x = 0; x < VIDEO_WIDTH; ++x)
        {
            const int cellX = x * cells / VIDEO_WIDTH;
            const int cellY = y * cells / VIDEO_HEIGHT;

            const bool light = ((cellX + cellY) & 1) == 0;

            // Full black / full brightness.
            unpackedFrame[y * VIDEO_WIDTH + x] =
                light ? 255 : 0;
        }
    }

    videoSetRasterMode();
}
void drawText(const char* text, int x, int y,
              int pixelSizeX, int pixelSizeY, int delayUs)
{
    const int charWidth = 6 * pixelSizeX;

    while (*text)
    {
        const Glyph* glyph = findGlyph(*text);

        if (glyph)
        {
            for (int row = 0; row < 7; ++row)
            {
                for (int col = 0; col < 5; ++col)
                {
                    if (!(glyph->rows[row] & (1 << (4 - col))))
                        continue;

                    const int px = x + col * pixelSizeX;
                    const int py = y + row * pixelSizeY;
                    
                    beamOn();
                    drawLine(
                        px, py,
                        px + pixelSizeX - 1, py,
                        delayUs
                    );
                    beamOff();
                }
            }
        }

        x += charWidth;
        ++text;
    }
}
void drawScrollingText(
    const char* text,
    int x,
    int y,
    int pixelSizeX,
    int pixelSizeY,
    int glyphBudgetUs = 15000)
{
    constexpr int visibleColumns = 20;
    constexpr int charWidth = 6;
    constexpr int maxCharacters = 128;

    // Decode UTF-8 string into Unicode codepoints.
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

            // Space or unknown glyph.
            if (lineCount == 0)
            {
                delayMicroseconds(glyphBudgetUs);
                continue;
            }

            // Equal total time budget for each glyph.
            const int delayUs = glyphBudgetUs / lineCount;

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

                    if (relativeCol >= visibleColumns)
                        continue;

                    const int px = x + relativeCol * pixelSizeX;
                    const int py = y + row * pixelSizeY;
                    
                    beamOn();
                    drawLine(
                        px, py,
                        px + pixelSizeX - 1, py,
                        delayUs
                    );
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