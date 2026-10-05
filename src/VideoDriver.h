
#pragma once

#include <Arduino.h>

constexpr int VIDEO_WIDTH = 100;
constexpr int VIDEO_HEIGHT = 100;
constexpr size_t VIDEO_FRAME_SIZE = VIDEO_WIDTH * VIDEO_HEIGHT;

void videoInit();
bool videoSubmitFrame(const uint8_t* frame, size_t length);
void videoStart();
void videoStop();