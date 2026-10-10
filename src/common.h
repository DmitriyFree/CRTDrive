#pragma once

constexpr int VIDEO_WIDTH = 100;
constexpr int VIDEO_HEIGHT = 100;
constexpr size_t VIDEO_FRAME_SIZE = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int SERIAL_TASK_STACK = 4096;
static constexpr int SERIAL_TASK_PRIORITY = 3;
static constexpr int VIDEO_PIXELS = VIDEO_WIDTH * VIDEO_HEIGHT;
static constexpr int PACKED2_SIZE = (VIDEO_PIXELS + 3) / 4;
static constexpr int SCAN_TASK_STACK    = 4096;
static constexpr int SCAN_TASK_PRIORITY = 19;
extern uint8_t unpackedFrame[VIDEO_PIXELS];


constexpr int X_PWM_PIN = 16;
constexpr int X_DIR_PIN = 17;
constexpr int Y_PWM_PIN = 18;
constexpr int Y_DIR_PIN = 19;
constexpr int DRIVER_SLEEP_PIN = 5;
constexpr int BEAM_ALPHA_A_PIN = 15;

constexpr int PWM_RESOLUTION_X = 12;
constexpr int PWM_RESOLUTION_Y = 10;
constexpr float X_GAIN = 0.085f;
constexpr float Y_GAIN = 0.38f;
constexpr int PWM_FREQUENCY_X = 19531;
constexpr int PWM_FREQUENCY_Y = 39062;

static constexpr int SETTLE_US = 40;
static constexpr int DWELL_STEP_nS = 500;
static constexpr int QUADRANTS = 32;

static constexpr int VECTOR_SETTLE_US = 5;