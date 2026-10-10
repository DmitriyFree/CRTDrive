
#pragma once

#include <Arduino.h>

void videoInit();
void videoStart();

void videoSetVectorFrame(const uint8_t* data, uint16_t len);
void videoSetRasterMode();