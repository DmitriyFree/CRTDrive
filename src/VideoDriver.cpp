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
        xPosition[col] = (-50 + col) * POS_MULTIPLIER_X;

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        yPosition[row] = (-50 + row) * POS_MULTIPLIER_Y;

        sourceRowOffset[row] =
            (VIDEO_HEIGHT - 1 - row) * VIDEO_WIDTH;
    }
}
static void scanFrame(const uint8_t* frame)
{
    constexpr int HALF_WIDTH = VIDEO_WIDTH / 2;

    // Four sequential quadrants:
    // 1. Top-left
    // 2. Top-right
    // 3. Bottom-left
    // 4. Bottom-right

    for (int quadrant = 0; quadrant < 4; ++quadrant)
    {
        const bool bottom = quadrant >= 2;
        const bool right  = quadrant & 1;

        const int rowStart =
            bottom ? VIDEO_HEIGHT / 2 : 0;

        const int rowEnd =
            bottom ? VIDEO_HEIGHT : VIDEO_HEIGHT / 2;

        const int colStart =
            right ? HALF_WIDTH : 0;

        const int colEnd =
            right ? VIDEO_WIDTH : HALF_WIDTH;

        for (int row = rowStart; row < rowEnd; ++row)
        {
            const uint8_t* src =
                frame + sourceRowOffset[row];

            // Preserve the normal snake pattern.
            const bool reverse = row & 1;

            int i;

            if (reverse)
                i = colEnd - 1;
            else
                i = colStart;

            while (reverse ? i >= colStart : i < colEnd)
            {
                // Find first lit pixel.
                int start = i;

                while (reverse ? start >= colStart : start < colEnd)
                {
                    const int col = start;

                    if ((src[col] >> 6) != 0)
                        break;

                    start += reverse ? -1 : 1;
                }

                if (reverse ? start < colStart : start >= colEnd)
                    break;

                // Find end of lit run.
                int end = start + (reverse ? -1 : 1);

                while (reverse ? end >= colStart : end < colEnd)
                {
                    const int col = end;

                    if ((src[col] >> 6) == 0)
                        break;

                    end += reverse ? -1 : 1;
                }

                // Move to run start while blanked.
                beamOff();

                setPoint(
                    xPosition[start],
                    yPosition[row]
                );

                delayMicroseconds(SETTLE_US);

                // Traverse the lit run.
                int p = start;

                while (reverse ? p != end : p != end)
                {
                    const int col = p;

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

                    p += reverse ? -1 : 1;
                }

                // Continue searching after this run.
                i = end;
            }

            if ((row + 1) % ROWS_PER_YIELD == 0)
                vTaskDelay(1);
        }
    }
}
static void scanFrame_(const uint8_t* frame)
{
    constexpr int PIXELS = VIDEO_WIDTH;

    beamOff();

    // Start at the left side.
    setPoint(
        xPosition[0],
        yPosition[0]
    );

    delayMicroseconds(SETTLE_US);

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        const uint8_t* src =
            frame + sourceRowOffset[row];

        beamOff();

        // Move to the START of this row.
        setPoint(
            xPosition[0],
            yPosition[row]
        );

        // Let the beam settle at the starting position.
        delayMicroseconds(SETTLE_US);

        // Now command the complete left -> right sweep.
        setPoint(
            xPosition[VIDEO_WIDTH - 1],
            yPosition[row]
        );

        const uint32_t sweepStart = micros();

        bool beamState = false;
        int lastPixel = -1;

        while (true)
        {
            const uint32_t elapsed =
                micros() - sweepStart;

            if (elapsed >= SWEEP_TIME_US)
                break;

            const int pixel =
                (int)(
                    (uint64_t)elapsed *
                    PIXELS /
                    SWEEP_TIME_US
                );

            if (pixel == lastPixel)
                continue;

            lastPixel = pixel;

            const int col = pixel;

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