#include <Arduino.h>

#include "SystemTypes.h"
#include "hardware.h"
#include "SerialProtocol.h"
#include "VideoDriver.h"

// ================= SERIAL =================

SerialProtocol SerialProto;
volatile DisplayMode mode = MODE_GRAPHICS;

void onFrameReceived(uint8_t* buf, uint16_t len)
{
    videoSubmitFrame(buf, len);
}

// ================= SETUP =================

void setup()
{
    Serial.setRxBufferSize(10100);
    Serial.begin(4000000);

    hardwareInit();
    videoInit();

    SerialProto.begin(
        Serial,
        onFrameReceived,
        nullptr,
        &mode
    );

    videoStart();
}

// ================= LOOP =================

void loop()
{
    SerialProto.update();
    vTaskDelay(1);
}