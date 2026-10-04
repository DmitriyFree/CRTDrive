#include <Arduino.h>

#include "hardware.h"
#include "drawing.h"

constexpr int SHAPE_SIZE = 200;

void setup()
{
    hardwareInit();

    
}

void loop()
{
    //showFigure(drawSquare, SHAPE_SIZE);
    //showFigure(drawTriangle, SHAPE_SIZE);
    //showFigure(drawCircle, SHAPE_SIZE);
    //showFilledSquare(SHAPE_SIZE);

    //showChessRaster(300);

    //showConcentricCircles();
    //showFigure(drawCross, SHAPE_SIZE);
    //showFigure(drawDiagonalCross, SHAPE_SIZE);
    //showFigure(drawDiamond, SHAPE_SIZE);

    //CLOCKS
    /*static unsigned long startTime = millis();
    const unsigned long elapsed = (millis() - startTime) / 10;

    const int hours   = (elapsed / 3600) % 24;
    const int minutes = (elapsed / 60) % 60;
    const int seconds = elapsed % 60;

    drawClock(hours, minutes, seconds);*/
    //drawAnalogClock(hours, minutes, seconds);
    

    // Dwell chess raster
    //drawDwellChessRaster(SHAPE_SIZE);

    //drawText("23:59", -30, 0, 4); //okay
    //drawScrollingText("June is awesome! ", 0, 0, 10);
    //drawPixelMatrix(0, 0, 64, 64, 1, 2);
    //drawChessMatrixAlpha(-100, -100, 100, 100, 2, 0, 2, 10);
    drawRotatingCube(20, 20, 150, 30);
}