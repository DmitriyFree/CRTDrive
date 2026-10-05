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
    digitalWrite(2, !digitalRead(2));
    videoSubmitFrame(buf, len);
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

    videoStart();
}

// ================= LOOP =================
void loop()
{
    SerialProto.update();
}