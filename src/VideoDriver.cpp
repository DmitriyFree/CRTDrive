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
    bool beamIsOn = false;

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        const uint8_t* src =
            frame + sourceRowOffset[row];

        const bool reverse = row & 1;

        for (int i = 0; i < VIDEO_WIDTH; ++i)
        {
            const int col =
                reverse
                    ? VIDEO_WIDTH - 1 - i
                    : i;

            const uint8_t dwell = src[col] >> 6;

            if (dwell == 0)
            {
                // Explicitly blank while traveling.
                if (beamIsOn)
                {
                    beamOff();
                    beamIsOn = false;
                }

                setPoint(
                    xPosition[col],
                    yPosition[row]
                );

                continue;
            }

            // Make absolutely sure the beam is off
            // before positioning at a lit pixel.
            if (beamIsOn)
            {
                beamOff();
                beamIsOn = false;
            }

            setPoint(
                xPosition[col],
                yPosition[row]
            );

            delayMicroseconds(SETTLE_US);

            beamOn();
            beamIsOn = true;

            delayMicroseconds(dwell);

            beamOff();
            beamIsOn = false;
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