#include "VideoDriver.h"
#include "hardware.h"
#include "common.h"
#include "drawing.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static TaskHandle_t scanTaskHandle = nullptr;
static int16_t xPosition[VIDEO_WIDTH];
static int16_t yPosition[VIDEO_HEIGHT];
static uint16_t sourceRowOffset[VIDEO_HEIGHT];
uint8_t unpackedFrame[VIDEO_PIXELS];
constexpr uint16_t MAX_VECTOR_SEGMENTS = 600;

struct VectorSegment
{
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
};

static VectorSegment vectorSegments[MAX_VECTOR_SEGMENTS];
static VectorSegment scanSegments[MAX_VECTOR_SEGMENTS];

static uint16_t vectorSegmentCount = 0;
static volatile uint8_t displayMode = 2; // 2 = raster, 3 = vector

static portMUX_TYPE displayMux = portMUX_INITIALIZER_UNLOCKED;

// Center the vector image around (0, 0).
constexpr float VECTOR_CENTER_X = 127.5f;
constexpr float VECTOR_CENTER_Y = 127.5f;
constexpr float VECTOR_SCALE_X = 3.5f * 200.0f / 255.0f;
constexpr float VECTOR_SCALE_Y = 3.5f * 200.0f / 255.0f;

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
        if (displayMode == 2)
        {
            // Existing raster scanner remains unchanged.
            scanFrame(unpackedFrame);
            continue;
        }

        // Snapshot the latest vector frame.
        uint16_t count;

        count = vectorSegmentCount;

        for (uint16_t i = 0; i < count; ++i)
            scanSegments[i] = vectorSegments[i];

        // Replay the same vector image continuously.
        if (count == 0)
        {
            //beamOff();
            vTaskDelay(1);
            continue;
        }

        
        for (uint16_t i = 0; i < count; ++i)
        {
            const VectorSegment& line = scanSegments[i];

            const int x1 = lroundf(
                (line.x1 - VECTOR_CENTER_X) * VECTOR_SCALE_X
            );
            const int y1 = lroundf(
                (line.y1 - VECTOR_CENTER_Y) * VECTOR_SCALE_Y
            );
            const int x2 = lroundf(
                (line.x2 - VECTOR_CENTER_X) * VECTOR_SCALE_X
            );
            const int y2 = lroundf(
                (line.y2 - VECTOR_CENTER_Y) * VECTOR_SCALE_Y
            );
            beamOn();
            drawLine(x1, -y1, x2, -y2, VECTOR_SETTLE_US);
            beamOff();
        }

        // Give other tasks a chance to run between replays.
        vTaskDelay(1);
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

void videoSetVectorFrame(const uint8_t* data, uint16_t len)
{
    if (data == nullptr || (len % 4) != 0)
        return;

    const uint16_t count = len / 4;

    if (count > MAX_VECTOR_SEGMENTS)
        return;

    for (uint16_t i = 0; i < count; ++i)
    {
        const uint16_t offset = i * 4;

        vectorSegments[i] = {
            data[offset],
            data[offset + 1],
            data[offset + 2],
            data[offset + 3]
        };
    }

    vectorSegmentCount = count;
    displayMode = 3;
}

void videoSetRasterMode()
{
    displayMode = 2;
}