#include <Arduino.h>
#include "VideoDriver.h"
#include "drawing.h"
#include "common.h"
#include "hardware.h"
#include "SerialProtocol.h"

SerialProtocol SerialProto;

void onFrameReceived(uint8_t* buf, uint16_t len, uint8_t mode)
{
    digitalWrite(2, !digitalRead(2));
    if (mode == 3)
    {
        // Each segment is X1, Y1, X2, Y2.
        const uint16_t segmentCount = len / 4;

        for (uint16_t i = 0; i < segmentCount; ++i)
        {
            const uint16_t offset = i * 4;

            const int x1 = buf[offset];
            const int y1 = buf[offset + 1];
            const int x2 = buf[offset + 2];
            const int y2 = buf[offset + 3];

            drawLine(x1, y1, x2, y2, 1);
        }

        return;
    }

    if (mode == 2)
    {
        if (len != PACKED2_SIZE)
            return;

        for (int i = 0; i < VIDEO_PIXELS; i++)
        {
            const uint8_t packed = buf[i >> 2];
            const uint8_t value =
                (packed >> ((i & 3) * 2)) & 0x03;

            unpackedFrame[i] = value * 85;
        }
    }
}

static void serialTask(void*)
{
    while (true)
    {
        SerialProto.update();
        vTaskDelay(1);
    }
}

// ================= SETUP =================

void setup()
{
    Serial.setRxBufferSize(10100);
    Serial.begin(3000000);

    hardwareInit();
    videoInit();

    SerialProto.begin(
        Serial,
        onFrameReceived
    );

    // CRT scanning → core 0
    videoStart();

    // Serial reception → core 1
    xTaskCreatePinnedToCore(
        serialTask,
        "serialRx",
        SERIAL_TASK_STACK,
        nullptr,
        SERIAL_TASK_PRIORITY,
        nullptr,
        1
    );
}

void loop(){
}