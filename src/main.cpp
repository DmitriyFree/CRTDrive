#include <Arduino.h>

#include "SystemTypes.h"
#include "hardware.h"
#include "SerialProtocol.h"
#include "VideoDriver.h"

// ================= SERIAL =================

SerialProtocol SerialProto;
volatile DisplayMode mode = MODE_GRAPHICS;

static constexpr int SERIAL_TASK_STACK = 4096;
static constexpr int SERIAL_TASK_PRIORITY = 3;

static TaskHandle_t serialTaskHandle = nullptr;

void onFrameReceived(uint8_t* buf, uint16_t len)
{
    digitalWrite(2, !digitalRead(2));
    videoSubmitFrame(buf, len);
}

static void serialTask(void*)
{
    while (true)
    {
        SerialProto.update();

        // Let other core-1 tasks run if there is currently no data.
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
        onFrameReceived,
        nullptr,
        &mode
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
        &serialTaskHandle,
        1
    );
}

// ================= LOOP =================

void loop()
{
    // Everything is handled by dedicated tasks.
    vTaskDelay(1000);
}