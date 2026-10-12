#include "VideoDriver.h"
#include "hardware.h"
#include "common.h"
#include "drawing.h"

#include <Arduino.h>
#include <cmath>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static TaskHandle_t scanTaskHandle = nullptr;
static int16_t xPosition[VIDEO_WIDTH];
static int16_t yPosition[VIDEO_HEIGHT];
static uint16_t sourceRowOffset[VIDEO_HEIGHT];

uint8_t unpackedFrame[VIDEO_PIXELS];

struct VectorSegment
{
    uint8_t x1, y1, x2, y2;
};

static VectorSegment vectorSegments[MAX_VECTOR_SEGMENTS];
static VectorSegment scanSegments[MAX_VECTOR_SEGMENTS];
static uint16_t vectorSegmentCount = 0;
static volatile uint8_t displayMode = 2;
static volatile bool outputEnabled = false;

constexpr float VECTOR_CENTER_X = 127.5f;
constexpr float VECTOR_CENTER_Y = 127.5f;

static inline void delayNanoseconds(uint32_t ns)
{
    constexpr uint32_t CPU_MHZ = 240;
    const uint32_t cycles = (ns * CPU_MHZ + 999) / 1000;
    const uint32_t start = esp_cpu_get_cycle_count();

    while (static_cast<uint32_t>(esp_cpu_get_cycle_count() - start) < cycles)
    {
    }
}

static void buildTables(float x_gain,float y_gain)
{
    const float xStep = pwmMaxDutyX() * x_gain / 50.0f;
    const float yStep = pwmMaxDutyY() * y_gain / 50.0f;

    for (int col = 0; col < VIDEO_WIDTH; ++col)
        xPosition[col] = static_cast<int>((col - 50) * xStep);

    for (int row = 0; row < VIDEO_HEIGHT; ++row)
    {
        yPosition[row] = static_cast<int>((row - 50) * yStep);
        sourceRowOffset[row] =
            (VIDEO_HEIGHT - 1 - row) * VIDEO_WIDTH;
    }
}

static void scanFrame(const uint8_t* frame)
{
    constexpr int X_SECTIONS = QUADRANTS / 2;
    constexpr int SECTION_WIDTH = VIDEO_WIDTH / (X_SECTIONS > 1 ? X_SECTIONS : 1);
    constexpr int HALF_HEIGHT = VIDEO_HEIGHT / 2;

    for (int section = 0; section < X_SECTIONS; ++section)
    {
        const int colStart = section * SECTION_WIDTH;
        const int colEnd = section == X_SECTIONS - 1
            ? VIDEO_WIDTH
            : colStart + SECTION_WIDTH;

        for (int half = 0; half < 2; ++half)
        {
            const int rowStart = half == 0 ? 0 : HALF_HEIGHT;
            const int rowEnd = half == 0 ? HALF_HEIGHT : VIDEO_HEIGHT;

            for (int row = rowStart; row < rowEnd; ++row)
            {
                const uint8_t* src = frame + sourceRowOffset[row];
                const bool reverse = row & 1;
                int i = reverse ? colEnd - 1 : colStart;

                while (reverse ? i >= colStart : i < colEnd)
                {
                    int start = i;

                    while (reverse ? start >= colStart : start < colEnd)
                    {
                        if (src[start] >> 6)
                            break;

                        start += reverse ? -1 : 1;
                    }

                    if (reverse ? start < colStart : start >= colEnd)
                        break;

                    int end = start + (reverse ? -1 : 1);

                    while (reverse ? end >= colStart : end < colEnd)
                    {
                        if (!(src[end] >> 6))
                            break;

                        end += reverse ? -1 : 1;
                    }

                    beamOff();
                    setPoint(xPosition[start], yPosition[row]);
                    delayMicroseconds(SETTLE_US);

                    for (int p = start; p != end; p += reverse ? -1 : 1)
                    {
                        if (p != start)
                            setX(xPosition[p]);

                        beamOn();
                        delayNanoseconds(DWELL_STEP_nS * (src[p] >> 6));
                        beamOff();
                    }

                    i = end;
                }
            }
        }
    }

    vTaskDelay(1);
}

static void videoScanTask(void*)
{
    DisplayProfile lastProfile = getDisplayProfile();

    while (true)
    {
        if (!outputEnabled)
        {
            vTaskDelay(1);
            continue;
        }

        const DisplayProfile wanted =
            displayMode == 2
                ? DisplayProfile::Raster
                : DisplayProfile::Vector;

        if (wanted != lastProfile)
        {
            if (!setDisplayProfile(wanted))
            {
                vTaskDelay(10);
                continue;
            }

            if(wanted == DisplayProfile::Raster)buildTables(RASTER_SCALE_X,RASTER_SCALE_Y);
            if(wanted == DisplayProfile::Vector)buildTables(VECTOR_SCALE_X,VECTOR_SCALE_Y);
            lastProfile = wanted;
        }

        if (displayMode == 2)
        {
            scanFrame(unpackedFrame);
            continue;
        }

        const uint16_t count = vectorSegmentCount;

        for (uint16_t i = 0; i < count; ++i)
            scanSegments[i] = vectorSegments[i];

        if (count == 0)
        {
            vTaskDelay(1);
            continue;
        }

        for (uint16_t i = 0; i < count; ++i)
        {
            const VectorSegment& line = scanSegments[i];

            const float vectorScaleX = (2.0f * pwmMaxDutyX()) / 255.0f;
            const float vectorScaleY = (2.0f * pwmMaxDutyY()) / 255.0f;

            const int x1 = lroundf((line.x1 - VECTOR_CENTER_X) * vectorScaleX);
            const int y1 = lroundf((line.y1 - VECTOR_CENTER_Y) * vectorScaleY);
            const int x2 = lroundf((line.x2 - VECTOR_CENTER_X) * vectorScaleX);
            const int y2 = lroundf((line.y2 - VECTOR_CENTER_Y) * vectorScaleY);

            beamOn();
            drawLine(x1, -y1, x2, -y2, VECTOR_SETTLE_US);
            beamOff();
        }

        vTaskDelay(1);
    }
}

void videoInit()
{
    memset(unpackedFrame, 255, sizeof(unpackedFrame));
    const DisplayProfile wanted =
            displayMode == 2
                ? DisplayProfile::Raster
                : DisplayProfile::Vector;

    if(wanted == DisplayProfile::Raster)buildTables(RASTER_SCALE_X,RASTER_SCALE_Y);
    if(wanted == DisplayProfile::Vector)buildTables(VECTOR_SCALE_X,VECTOR_SCALE_Y);
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
    if (!data || len % 4 != 0)
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
    videoEnable();
}

void videoSetRasterMode()
{
    displayMode = 2;
}

void videoEnable()
{
    outputEnabled = true;
}

void videoDisable()
{
    outputEnabled = false;
}