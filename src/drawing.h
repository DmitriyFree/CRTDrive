#pragma once

void drawDigit(int digit, int x, int y, int w, int h);
void drawClock(int hours, int minutes, int seconds);

void drawAnalogClock(int hours, int minutes, int seconds);

void drawDwellChessRaster(int size);

void drawText(const char* text, int x, int y, int pixelSize);

void drawLine(int x1, int y1, int x2, int y2);

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