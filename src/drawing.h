#pragma once

void drawDigit(int digit, int x, int y, int w, int h);
void drawClock(int hours, int minutes, int seconds);

void drawAnalogClock(int hours, int minutes, int seconds);

void drawDwellChessRaster(int size);

void drawText(const char* text, int x, int y, int pixelSize);
void drawScrollingText(const char* text, int x, int y, int pixelSize);
void drawPixelMatrix(int x, int y, int columns, int rows, int pixelSize, int spacing);
void drawChessMatrix(
    int x,
    int y,
    int columns,
    int rows,
    int pixelSize,
    int spacing,
    int delayUs,
    int cellsize
);
void drawChessMatrixAlpha(
    int x,
    int y,
    int columns,
    int rows,
    int pixelSize,
    int spacing,
    int delayUs,
    int cellsize
);
void drawRotatingCube(int cx, int cy, int size, int delayUs);
void drawBitmapAlpha(
    const unsigned char* bitmap,
    int delayUs
);
void drawLine(int x1, int y1, int x2, int y2, int delayUs);

void drawSquare(int size);
void drawTriangle(int size);
void drawCircle(int radius);
void drawFilledSquare(int size);

void drawCross(int size);
void drawDiagonalCross(int size);
void drawDiamond(int size);

void drawConcentricCircles();

void showFigure(void (*figure)(int), int size);
void showConcentricCircles();
void showFilledSquare(int size);

void drawChessRaster(int size);
void showChessRaster(int size);