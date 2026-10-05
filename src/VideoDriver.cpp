#include "VideoDriver.h"
#include "hardware.h"
#include "common.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static TaskHandle_t scanTaskHandle = nullptr;
static int16_t xPosition[VIDEO_WIDTH];
static int16_t yPosition[VIDEO_HEIGHT];
static uint16_t sourceRowOffset[VIDEO_HEIGHT];
static uint8_t dwellTable[256];

static void buildTables()
{
    for (int i = 0; i < 256; ++i)
        dwellTable[i] = (i >> 6) * DWELL_STEP_US;

    for (int col = 0; col < VIDEO_WIDTH; ++col)
        xPosition[col] = (-50 + col) * 2;

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        yPosition[row] = (-50 + row) * 2;

        sourceRowOffset[row] =
            (VIDEO_HEIGHT - 1 - row) * VIDEO_WIDTH;
    }
}
static void scanFrame(const uint8_t* frame)
{
    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        const uint8_t* src =
            frame + sourceRowOffset[row];

        const bool reverse = row & 1;

        for (int i = 0; i < VIDEO_WIDTH; ++i)
        {
            // Serpentine scan.
            const int col =
                reverse
                    ? VIDEO_WIDTH - 1 - i
                    : i;

            const uint8_t pixel = src[col];
            const uint8_t dwell = dwellTable[pixel];

            if (dwell == 0)
                continue;

            setPoint(
                xPosition[col],
                yPosition[row]
            );

            delayMicroseconds(SETTLE_US);

            beamOn();
            delayMicroseconds(dwell);
            beamOff();
        }

        // Give core 0's FreeRTOS tasks a chance to run.
        if ((row + 1) % ROWS_PER_YIELD == 0)
            vTaskDelay(1);
    }
}
static void videoScanTask(void*)
{
    while (true)
    {
        scanFrame(unpackedFrame);
    }
}
void videoInit()
{
    buildTables();
}
void videoStart()
{
    xTaskCreatePinnedToCore(
        videoScanTask,
        "videoScan",
        SCAN_TASK_STACK,
        nullptr,
        SCAN_TASK_PRIORITY,
        &scanTaskHandle,
        1
    );
}