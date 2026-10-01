#pragma once

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