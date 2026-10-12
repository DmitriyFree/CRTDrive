#pragma once
#include <stdint.h>

enum class DisplayProfile : uint8_t
{
    Raster,
    Vector,
};

struct PwmProfile
{
    uint32_t frequency;
    uint8_t resolutionBits;
    float xScale;
    float yScale;
};

void setDisplayProfile(DisplayProfile profile);
DisplayProfile getDisplayProfile();