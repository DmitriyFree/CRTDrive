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


static inline void delayNanoseconds(uint32_t ns)
{
    constexpr uint32_t CPU_MHZ = 240;
    const uint32_t cycles = (ns * CPU_MHZ + 999) / 1000;

    const uint32_t start = esp_cpu_get_cycle_count();

    while ((uint32_t)(esp_cpu_get_cycle_count() - start) < cycles)
    {
        // Busy wait.
    }
}

static void buildTables()
{
    constexpr int X_MAX_DUTY = (1 << PWM_RESOLUTION_X) - 1;
    constexpr int Y_MAX_DUTY = (1 << PWM_RESOLUTION_Y) - 1;

    const float xScale = (X_MAX_DUTY * X_GAIN) / 50.0f;
    const float yScale = (Y_MAX_DUTY * Y_GAIN) / 50.0f;

    for (int col = 0; col < VIDEO_WIDTH; ++col)
        xPosition[col] = (int)((-50 + col) * xScale);

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        yPosition[row] = (int)((-50 + row) * yScale);

        sourceRowOffset[row] =
            (VIDEO_HEIGHT - 1 - row) * VIDEO_WIDTH;
    }
}

static void scanFrame(const uint8_t* frame)
{
    constexpr int X_SECTIONS = QUADRANTS / 2;
    constexpr int SECTION_WIDTH =
        VIDEO_WIDTH / (X_SECTIONS > 1 ? X_SECTIONS : 1);

    constexpr int HALF_HEIGHT = VIDEO_HEIGHT / 2;

    for (int section = 0; section < X_SECTIONS; ++section)
    {
        const int colStart = section * SECTION_WIDTH;
        const int colEnd =
            (section == X_SECTIONS - 1)
                ? VIDEO_WIDTH
                : colStart + SECTION_WIDTH;

        for (int half = 0; half < 2; ++half)
        {
            const int rowStart = half == 0 ? 0 : HALF_HEIGHT;
            const int rowEnd = half == 0 ? HALF_HEIGHT : VIDEO_HEIGHT;

            for (int row = rowStart; row < rowEnd; ++row)
            {
                const uint8_t* src =
                    frame + sourceRowOffset[row];

                const bool reverse = row & 1;
                int i = reverse ? colEnd - 1 : colStart;

                while (reverse ? i >= colStart : i < colEnd)
                {
                    // Find the next lit pixel.
                    int start = i;

                    while (reverse ? start >= colStart : start < colEnd)
                    {
                        if ((src[start] >> 6) != 0)
                            break;

                        start += reverse ? -1 : 1;
                    }

                    if (reverse ? start < colStart : start >= colEnd)
                        break;

                    // Find the end of this lit run.
                    int end = start + (reverse ? -1 : 1);

                    while (reverse ? end >= colStart : end < colEnd)
                    {
                        if ((src[end] >> 6) == 0)
                            break;

                        end += reverse ? -1 : 1;
                    }

                    // Reposition with the beam blanked.
                    beamOff();
                    setPoint(xPosition[start], yPosition[row]);
                    delayMicroseconds(SETTLE_US);

                    // Y remains fixed throughout this run.
                    int p = start;

                    while (p != end)
                    {
                        // setPoint() already positioned X at start.
                        // Only update X for subsequent pixels.
                        if (p != start)
                            setX(xPosition[p]);

                        const uint8_t dwell = src[p] >> 6;

                        beamOn();
                        delayNanoseconds(DWELL_STEP_nS * dwell);
                        beamOff();

                        p += reverse ? -1 : 1;
                    }

                    i = end;
                }
            }
        }
    }

    // Yield once per frame rather than once per section.
    vTaskDelay(1);
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