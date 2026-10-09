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

void delayNanoseconds(uint32_t ns) {
    // 240 MHz = 240 cycles per microsecond = 0.24 cycles per nanosecond
    // Multiply by 240 and divide by 1000 to get required cycles
    uint32_t cycles = (ns * 240) / 1000; 

    uint32_t start = esp_cpu_get_cycle_count();
    while ((esp_cpu_get_cycle_count() - start) < cycles) {
        // Busy wait
    }
}

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
    // Number of scan sections.
    //
    // 2  = right half, then left half
    // 4  = right-top, right-bottom, left-top, left-bottom
    // 8  = 4 X sections, each split into top/bottom
    // 16 = 8 X sections, each split into top/bottom
    // 32 = 16 X sections, each split into top/bottom
    //
    // Must be a multiple of 2.

    constexpr int X_SECTIONS = QUADRANTS / 2;
    constexpr int SECTION_WIDTH =
        VIDEO_WIDTH / (X_SECTIONS > 1 ? X_SECTIONS:1);

    constexpr int HALF_HEIGHT =
        VIDEO_HEIGHT / 2;

    // Start from RIGHT and move toward LEFT.
    for (int section = 0;
         section < X_SECTIONS;
         ++section)
    {
        const int colStart =
            section * SECTION_WIDTH;

        const int colEnd =
            (section == X_SECTIONS - 1)
                ? VIDEO_WIDTH
                : colStart + SECTION_WIDTH;

        // Top, then bottom.
        for (int half = 0; half < 2; ++half)
        {
            const int rowStart =
                half == 0
                    ? 0
                    : HALF_HEIGHT;

            const int rowEnd =
                half == 0
                    ? HALF_HEIGHT
                    : VIDEO_HEIGHT;

            for (int row = rowStart;
                 row < rowEnd;
                 ++row)
            {
                const uint8_t* src =
                    frame + sourceRowOffset[row];

                // Preserve snake pattern.
                const bool reverse = row & 1;

                int i =
                    reverse
                        ? colEnd - 1
                        : colStart;

                while (reverse
                    ? i >= colStart
                    : i < colEnd)
                {
                    // Find first lit pixel.
                    int start = i;

                    while (reverse
                        ? start >= colStart
                        : start < colEnd)
                    {
                        if ((src[start] >> 6) != 0)
                            break;

                        start += reverse ? -1 : 1;
                    }

                    if (reverse
                        ? start < colStart
                        : start >= colEnd)
                    {
                        break;
                    }

                    // Find end of lit run.
                    int end =
                        start + (reverse ? -1 : 1);

                    while (reverse
                        ? end >= colStart
                        : end < colEnd)
                    {
                        if ((src[end] >> 6) == 0)
                            break;

                        end += reverse ? -1 : 1;
                    }

                    // Move to beginning of lit run
                    // with beam blanked.
                    beamOff();

                    setPoint(
                        xPosition[start],
                        yPosition[row]
                    );

                    delayMicroseconds(SETTLE_US);

                    // Draw the lit run.
                    int p = start;

                    while (p != end)
                    {
                        const uint8_t dwell =
                            src[p] >> 6;

                        setPoint(
                            xPosition[p],
                            yPosition[row]
                        );

                        if (dwell != 0)
                        {
                            beamOn();

                            delayNanoseconds(DWELL_STEP_nS * dwell);

                            beamOff();
                        }

                        p += reverse ? -1 : 1;
                    }

                    i = end;
                }

                if ((row + 1) % ROWS_PER_YIELD == 0)
                    vTaskDelay(1);
            }
        }
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