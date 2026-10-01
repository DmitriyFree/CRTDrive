#include <Arduino.h>

#include "hardware.h"
#include "drawing.h"

constexpr int SHAPE_SIZE = 100;

void setup()
{
    hardwareInit();
}

void loop()
{
    /*showFigure(drawSquare, SHAPE_SIZE);
    showFigure(drawTriangle, SHAPE_SIZE);
    showFigure(drawCircle, SHAPE_SIZE);
    showConcentricCircles();
    showFigure(drawCross, SHAPE_SIZE);
    showFigure(drawDiagonalCross, SHAPE_SIZE);
    showFigure(drawDiamond, SHAPE_SIZE);*/
    //showFilledSquare();
    showChessRaster(SHAPE_SIZE);
}