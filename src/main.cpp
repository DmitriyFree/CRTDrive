#include <Arduino.h>

#include "hardware.h"
#include "drawing.h"

constexpr int SHAPE_SIZE = 140;

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

    /*//CLOCKS
    static unsigned long startTime = millis();
    const unsigned long elapsed = (millis() - startTime) / 1000;

    const int hours   = (elapsed / 3600) % 24;
    const int minutes = (elapsed / 60) % 60;
    const int seconds = elapsed % 60;

    //drawClock(hours, minutes, seconds);
    //drawAnalogClock(hours, minutes, seconds);
    */
   
    // Dwell chess raster
    drawDwellChessRaster(SHAPE_SIZE);
}