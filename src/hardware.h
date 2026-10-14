#pragma once

#include <stdint.h>

enum class DisplayProfile : uint8_t
{
    Raster,
    Vector,
    Text
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

inline int compensateDeadZone(int value, int deadZone)
{
    if (value >= 0)
        return value + deadZone;

    if (value < 0)
        return value - deadZone;

    //return 0;
}
int getDisplayDeadZoneX();
int getDisplayDeadZoneY();