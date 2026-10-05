
#pragma once

#include <Arduino.h>


void videoInit();
bool videoSubmitFrame(const uint8_t* frame, size_t length);
void videoStart();
void videoStop();