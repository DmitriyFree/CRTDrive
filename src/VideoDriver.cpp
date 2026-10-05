
#include "VideoDriver.h"
#include "hardware.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// CRT scan configuration
static constexpr int SETTLE_US = 10;
static constexpr int DWELL_STEP_US = 5;
static constexpr int SCAN_TASK_STACK = 4096;
static constexpr int SCAN_TASK_PRIORITY = 3;

// Two grayscale frame buffers
static uint8_t frames[2][VIDEO_FRAME_SIZE];

static int activeBuffer = 0;
static int pendingBuffer = 1;
static bool hasPendingFrame = false;

static SemaphoreHandle_t frameMutex = nullptr;
static TaskHandle_t scanTaskHandle = nullptr;
static volatile bool running = false;

// Map grayscale to six levels:
// 0, 2, 4, 6, 8, 10 micros
static inline int pixelDwellUs(uint8_t pixel)
{
    const int level = pixel >> 6; // 64; 4 colors
    return level * DWELL_STEP_US;
}

static void scanFrame(const uint8_t* frame)
{
    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        for (int i = 0; i < VIDEO_WIDTH; ++i)
        {
            const int col = (row % 2 == 0)
                ? i
                : VIDEO_WIDTH - 1 - i;

            const int sourceRow = VIDEO_HEIGHT - 1 - row;
            const uint8_t pixel =
                frame[sourceRow * VIDEO_WIDTH + col];

            // Skip black and values quantized to level zero
            const int dwellUs = pixelDwellUs(pixel);
            if (dwellUs == 0)
                continue;

            const int x = (-50 + col) * 2;
            const int y = (-50 + row) * 2;

            setPoint(x, y);
            delayMicroseconds(SETTLE_US);
            beamOn();
            delayMicroseconds(dwellUs);
            beamOff();
        }
    }
}

static void videoScanTask(void*)
{
    while (true)
    {
        if (!running)
        {
            vTaskDelay(1);
            continue;
        }

        xSemaphoreTake(frameMutex, portMAX_DELAY);

        if (hasPendingFrame)
        {
            activeBuffer = pendingBuffer;
            pendingBuffer = 1 - activeBuffer;
            hasPendingFrame = false;
        }

        const uint8_t* frame = frames[activeBuffer];

        xSemaphoreGive(frameMutex);

        scanFrame(frame);

        // Give FreeRTOS/idle task a chance to run.
        vTaskDelay(1);
    }
}

void videoInit()
{
    frameMutex = xSemaphoreCreateMutex();

    memset(frames, 255, sizeof(frames));

    activeBuffer = 0;
    pendingBuffer = 1;
    hasPendingFrame = false;
    running = false;
}

bool videoSubmitFrame(const uint8_t* frame, size_t length)
{
    if (!frame || length != VIDEO_FRAME_SIZE || !frameMutex)
        return false;

    xSemaphoreTake(frameMutex, portMAX_DELAY);

    // The active buffer is never written while being scanned.
    memcpy(frames[pendingBuffer], frame, VIDEO_FRAME_SIZE);
    hasPendingFrame = true;

    xSemaphoreGive(frameMutex);
    return true;
}

void videoStart()
{
    if (scanTaskHandle)
        return;

    running = true;

    xTaskCreatePinnedToCore(
        videoScanTask,
        "videoScan",
        SCAN_TASK_STACK,
        nullptr,
        SCAN_TASK_PRIORITY,
        &scanTaskHandle,
        0
    );
}

void videoStop()
{
    running = false;
    beamOff();
}