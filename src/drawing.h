
#pragma once

void drawLine(int x1, int y1, int x2, int y2, int delayUs = 0);

void drawSquare(int size, int delayUs);
void drawTriangle(int size, int delayUs);
void drawCircle(int radius, int delayUs);
void drawFilledSquare(int size);
void drawCross(int size, int delayUs);
void drawDiagonalCross(int size, int delayUs);
void drawDiamond(int size, int delayUs);
void drawConcentricCircles(int delayUs);

void drawClock(int hours, int minutes, int seconds, int delayUs);
void drawAnalogClock(int hours, int minutes, int seconds, int delayUs);

void drawChess(int cells);

void drawText(const char* text, int x, int y, int pixelSizeX, int pixelSizeY, int delayUs);
void drawScrollingText(const char* text, int x, int y, int pixelSizeX, int pixelSizeY, int glyphBudgetUs);
void drawRotatingCube(int cx, int cy, int size, int delayUs);