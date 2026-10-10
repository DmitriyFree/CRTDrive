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

    // false: top-left -> bottom-right
    // true:  bottom-right -> top-left
    static bool reverseFrame = false;

    beamOff();

    for (int rowStep = 0; rowStep < VIDEO_HEIGHT; ++rowStep)
    {
        // Alternate the vertical scan direction every frame.
        const int row = reverseFrame
            ? VIDEO_HEIGHT - 1 - rowStep
            : rowStep;

        const uint8_t* src = frame + sourceRowOffset[row];

        // Alternate horizontal direction on every row.
        // The first row starts at the appropriate frame corner.
        const bool reverseX = ((rowStep & 1) != 0) ^ reverseFrame;

        bool firstPixel = true;

        // Traverse the existing X sections in scan order.
        for (int sectionStep = 0;
             sectionStep < X_SECTIONS;
             ++sectionStep)
        {
            const int section = reverseX
                ? X_SECTIONS - 1 - sectionStep
                : sectionStep;

            const int colStart = section * SECTION_WIDTH;
            const int colEnd =
                (section == X_SECTIONS - 1)
                    ? VIDEO_WIDTH
                    : colStart + SECTION_WIDTH;

            int x = reverseX ? colEnd - 1 : colStart;
            const int xEnd = reverseX ? colStart - 1 : colEnd;
            const int xStep = reverseX ? -1 : 1;

            for (; x != xEnd; x += xStep)
            {
                // At the start of each row, set both axes.
                // Subsequent pixels only change X.
                if (firstPixel)
                {
                    setPoint(xPosition[x], yPosition[row]);
                    firstPixel = false;
                }
                else
                {
                    setX(xPosition[x]);
                }

                const uint8_t level = src[x] >> 6;

                if (level != 0)
                {
                    beamOn();
                    delayNanoseconds(DWELL_STEP_nS * level);
                    beamOff();
                }
                else
                {
                    beamOff();

#if !SKIP_BLACK_PIXELS
                    // Optional: preserve a small timing interval for
                    // black pixels too, while keeping the beam blanked.
                    delayNanoseconds(DWELL_STEP_nS);
#endif
                }
            }
        }
    }

    // Next frame starts from the opposite corner.
    reverseFrame = !reverseFrame;

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