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

constexpr int PWM_RESOLUTION_X = 9;  // Initial hardware setup only
constexpr int PWM_RESOLUTION_Y = 9;

// Per-profile output compensation.
constexpr float RASTER_SCALE_X = 0.25f;
constexpr float RASTER_SCALE_Y = 0.55f;
constexpr float VECTOR_SCALE_X = 1.6f;
constexpr float VECTOR_SCALE_Y = 1.6f;
constexpr float TEXT_SCALE_X = 0.6f;
constexpr float TEXT_SCALE_Y = 0.6f;

static constexpr int DELAY_US = 30;
static constexpr int SETTLE_US = 25;
static constexpr int DWELL_STEP_nS = 500;
static constexpr int QUADRANTS = 32;

static constexpr int VECTOR_SETTLE_US = 3;
constexpr uint16_t MAX_VECTOR_SEGMENTS = 600;