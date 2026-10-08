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
uint8_t unpackedFrame[VIDEO_PIXELS];

static void buildTables()
{
    for (int col = 0; col < VIDEO_WIDTH; ++col)
        xPosition[col] = (-50 + col) * POS_MULTIPLIER;

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        yPosition[row] = (-50 + row) * POS_MULTIPLIER;

        sourceRowOffset[row] =
            (VIDEO_HEIGHT - 1 - row) * VIDEO_WIDTH;
    }
}
static void scanFrame(const uint8_t* frame)
{
    constexpr int PIXELS = VIDEO_WIDTH;

    // Start at the left side of the first row.
    beamOff();

    setPoint(
        xPosition[0],
        yPosition[0]
    );

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        const uint8_t* src =
            frame + sourceRowOffset[row];

        const bool reverse = row & 1;

        const int endCol =
            reverse
                ? 0
                : VIDEO_WIDTH - 1;

        // Command the entire next-row movement.
        beamOff();

        setPoint(
            xPosition[endCol],
            yPosition[row]
        );

        const uint32_t sweepStart = micros();

        bool beamState = false;
        int lastPixel = -1;

        while (true)
        {
            const uint32_t elapsed =
                micros() - sweepStart;

            if (elapsed >= SETTLE_US)
                break;

            // Convert time into pixel index.
            //
            // elapsed:
            // 0 ................ SETTLE_US
            //
            // pixel:
            // 0 ................ VIDEO_WIDTH-1
            const int pixel =
                (int)(
                    (uint64_t)elapsed *
                    PIXELS /
                    SETTLE_US
                );

            if (pixel == lastPixel)
                continue;

            lastPixel = pixel;

            const int col =
                reverse
                    ? VIDEO_WIDTH - 1 - pixel
                    : pixel;

            const bool shouldBeOn =
                (src[col] >> 6) != 0;

            if (shouldBeOn != beamState)
            {
                beamState = shouldBeOn;

                if (beamState)
                    beamOn();
                else
                    beamOff();
            }
        }

        beamOff();

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
    memset(unpackedFrame, 255, sizeof(unpackedFrame));
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
        0
    );
}