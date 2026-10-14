#pragma once

#include <stdint.h>

enum class DisplayProfile : uint8_t
{
    Raster,
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
    if (deadZone <= 0)
        return value;

    return value < 0
        ? value - deadZone
        : value + deadZone;
}
int getDisplayDeadZoneX();
int getDisplayDeadZoneY();