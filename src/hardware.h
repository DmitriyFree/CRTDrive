#pragma once

#include <stdint.h>

enum class DisplayProfile : uint8_t
{
    Raster,
    Vector,
};

void hardwareInit();

bool setDisplayProfile(DisplayProfile profile);
DisplayProfile getDisplayProfile();

int pwmMaxDutyX();
int pwmMaxDutyY();
float pwmScaleX();
float pwmScaleY();

void setX(int x);
void setY(int y);
void setPoint(int x, int y);

void beamOn();
void beamOff();