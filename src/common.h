#pragma once

constexpr int VIDEO_WIDTH = 100;
constexpr int VIDEO_HEIGHT = 100;
constexpr size_t VIDEO_FRAME_SIZE = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int SERIAL_TASK_STACK = 4096;
static constexpr int SERIAL_TASK_PRIORITY = 3;
static constexpr int VIDEO_PIXELS = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int PACKED2_SIZE = (VIDEO_PIXELS + 3) / 4;
static constexpr int SETTLE_US       = 0;
static constexpr int DWELL_STEP_US   = 1;
static constexpr int SCAN_TASK_STACK    = 4096;
static constexpr int SCAN_TASK_PRIORITY = 3;
static constexpr int ROWS_PER_YIELD = 4;
extern uint8_t unpackedFrame[VIDEO_PIXELS];
static constexpr float POS_MULTIPLIER = 1.8;