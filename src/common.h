#pragma once

constexpr int VIDEO_WIDTH = 100;
constexpr int VIDEO_HEIGHT = 100;
constexpr size_t VIDEO_FRAME_SIZE = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int SERIAL_TASK_STACK = 4096;
static constexpr int SERIAL_TASK_PRIORITY = 3;
static constexpr int VIDEO_PIXELS = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int PACKED2_SIZE = (VIDEO_PIXELS + 3) / 4;
static constexpr int SCAN_TASK_STACK    = 4096;
static constexpr int SCAN_TASK_PRIORITY = 3;
static constexpr int ROWS_PER_YIELD = 25;
extern uint8_t unpackedFrame[VIDEO_PIXELS];
static constexpr float POS_MULTIPLIER = 1.8;

constexpr int X_PWM_PIN = 16;
constexpr int X_DIR_PIN = 17;
constexpr int Y_PWM_PIN = 18;
constexpr int Y_DIR_PIN = 19;
constexpr int DRIVER_SLEEP_PIN = 5;
constexpr int BEAM_ALPHA_A_PIN = 15;
constexpr int PWM_RESOLUTION = 8;

constexpr int PWM_FREQUENCY = 30000;
static constexpr int SETTLE_US       = 50;
static constexpr int DWELL_STEP_US   = 2;