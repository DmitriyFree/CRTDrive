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
    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        const uint8_t* src =
            frame + sourceRowOffset[row];

        const bool reverse = row & 1;

        int i = 0;

        while (i < VIDEO_WIDTH)
        {
            // Find first lit pixel.
            int start = i;

            while (start < VIDEO_WIDTH)
            {
                const int col =
                    reverse
                        ? VIDEO_WIDTH - 1 - start
                        : start;

                if ((src[col] >> 6) != 0)
                    break;

                ++start;
            }

            if (start >= VIDEO_WIDTH)
                break;

            // Find end of lit run.
            int end = start + 1;

            while (end < VIDEO_WIDTH)
            {
                const int col =
                    reverse
                        ? VIDEO_WIDTH - 1 - end
                        : end;

                if ((src[col] >> 6) == 0)
                    break;

                ++end;
            }

            // Move to run start while blanked.
            const int startCol =
                reverse
                    ? VIDEO_WIDTH - 1 - start
                    : start;

            beamOff();

            setPoint(
                xPosition[startCol],
                yPosition[row]
            );

            delayMicroseconds(SETTLE_US);

            // Now traverse the run.
            for (int p = start; p < end; ++p)
            {
                const int col =
                    reverse
                        ? VIDEO_WIDTH - 1 - p
                        : p;

                setPoint(
                    xPosition[col],
                    yPosition[row]
                );

                const uint8_t dwell =
                    src[col] >> 6;

                if (dwell != 0)
                {
                    beamOn();
                    delayMicroseconds(
                        dwell * DWELL_STEP_US
                    );
                    beamOff();
                }
            }

            i = end;
        }

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