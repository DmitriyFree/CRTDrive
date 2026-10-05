#include <Arduino.h>

#include "common.h"
#include "hardware.h"
#include "SerialProtocol.h"
#include "VideoDriver.h"

SerialProtocol SerialProto;

void onFrameReceived(uint8_t* buf, uint16_t len)
{
    digitalWrite(2, !digitalRead(2));
    for (int i = 0; i < VIDEO_PIXELS; i++)
    {
        uint8_t packed = buf[i >> 2];
        uint8_t value = (packed >> ((i & 3) * 2)) & 0x03;

        unpackedFrame[i] = value * 85;
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
        0
    );
}

// ================= LOOP =================

void loop()
{
    // Everything is handled by dedicated tasks.
    //vTaskDelay(1000);
}