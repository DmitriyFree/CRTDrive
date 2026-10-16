
#include <Arduino.h>

#include "VideoDriver.h"
#include "drawing.h"
#include "common.h"
#include "hardware.h"
#include "SerialProtocol.h"
#include <cstdio>

SerialProtocol SerialProto;

constexpr uint32_t VIDEO_TIMEOUT_MS = 5000;
constexpr uint32_t TEST_DURATION_MS = 2500;

static uint32_t lastFrameMillis = 0;
static bool showingVideo = false;
static uint32_t testStartMillis = 0;
static uint16_t currentTest = 0;

static int clockHours = 10;
static int clockMinutes = 0;
static int clockSeconds = 0;
static uint32_t clockStartMillis = 0;

// -----------------------------------------------------------------------------
// Test wrappers
// -----------------------------------------------------------------------------

static void testSquare()
{
    drawSquare(150, DELAY_US);
}

static void testCircle()
{
    drawCircle(250, DELAY_US);
}

static void testTriangle()
{
    drawTriangle(180, DELAY_US);
}

static void testCross()
{
    drawCross(200, DELAY_US);
}

static void drawDiagonalCross()
{
    drawDiagonalCross(200, DELAY_US);
}

static void testFilledSquare()
{
    drawFilledSquare(100);
}

static void testDiamond()
{
    drawDiamond(100, DELAY_US);
}

static void testAnalogClock()
{
    const uint32_t elapsed = (millis() - clockStartMillis) / 50;

    clockHours = 10 + (elapsed / 3600) % 12;
    clockMinutes = (elapsed / 60) % 60;
    clockSeconds = elapsed % 60;

    drawAnalogClock(
        clockHours, clockMinutes, clockSeconds, DELAY_US
    );
}

static void testDigitalClock()
{
    const uint32_t elapsed = (millis() - clockStartMillis) / 50;

    clockHours = 23 + (elapsed / 3600) % 12;
    clockMinutes = 20 + (elapsed / 60) % 60;
    clockSeconds = elapsed % 60;

    drawClock(clockHours, clockMinutes, clockSeconds, 7);
}

static void testText()
{
    drawText("June", -180, -90, 15, 15, 40);
    drawText(" CRT", -180, 50, 15, 15, 40);
}

static void testScrollingText()
{
    drawScrollingText("June is awesome! Like, subscribe! ", -220, -50, 15, 15, 500);
}

static void testRotatingCube()
{
    drawRotatingCube(0, 0, 400, DELAY_US/2);
}

static void testChessboard()
{
    drawChess(5);
}

// -----------------------------------------------------------------------------
// Test registry
// -----------------------------------------------------------------------------

struct DemoTest
{
    const char* name;
    void (*render)();
    DisplayProfile profile;
    bool usesVideoScanner;
};

static const DemoTest tests[] = {
    // Uncomment or comment entries to choose the test sequence.
    {"Text",            testText,          DisplayProfile::Text, false},
    {"Square",        testSquare,        DisplayProfile::Text, false},
    {"Circle",        testCircle,        DisplayProfile::Text, false},
    {"Cross",         testCross,         DisplayProfile::Text, false},
    {"Rotating cube", testRotatingCube,  DisplayProfile::Text, false},
    {"Analog clock",  testAnalogClock,   DisplayProfile::Text, false},
    {"Digital clock", testDigitalClock,  DisplayProfile::Text, false},
   //{"DiagonalCross",         drawDiagonalCross,         DisplayProfile::Text, false},
     //{"Triangle",      testTriangle,      DisplayProfile::Text, false},
     //{"Filled square", testFilledSquare,  DisplayProfile::Text, false},//sus
     //{"Diamond",       testDiamond,       DisplayProfile::Text, false},//sus
     //{"Rotating cube", testRotatingCube,  DisplayProfile::Text, false},
    {"ScrollingText",   testScrollingText, DisplayProfile::Text, false},
    {"Chessboard",    testChessboard,    DisplayProfile::Raster,  true},
     
};

constexpr uint16_t TEST_COUNT = sizeof(tests) / sizeof(tests[0]);

// -----------------------------------------------------------------------------
// Serial reception
// -----------------------------------------------------------------------------

void onFrameReceived(uint8_t* buf, uint16_t len, uint8_t mode)
{
    if (buf == nullptr)
        return;

    bool validFrame = false;

    if (mode == 3)
    {
        if ((len % 4) != 0 || len / 4 > MAX_VECTOR_SEGMENTS)
            return;

        videoSetVectorFrame(buf, len);
        validFrame = true;
    }
    else if (mode == 2)
    {
        if (len != PACKED2_SIZE)
            return;

        for (int i = 0; i < VIDEO_PIXELS; ++i)
        {
            const uint8_t packed = buf[i >> 2];
            const uint8_t value =
                (packed >> ((i & 3) * 2)) & 0x03;

            unpackedFrame[i] = value * 85;
        }

        //for (size_t i = 0; i < VIDEO_PIXELS; ++i)unpackedFrame[i] ^= 0xC0;//invert(optional)

        videoSetRasterMode();
        validFrame = true;
    }

    if (validFrame)
    {
        lastFrameMillis = millis();
        showingVideo = true;

        // The scanner selects Raster or Vector profile.
        videoEnable();

        digitalWrite(2, !digitalRead(2));
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

// -----------------------------------------------------------------------------
// Demo sequence
// -----------------------------------------------------------------------------

static void startTest(uint16_t index)
{
    if (index >= TEST_COUNT)
        index = 0;

    // Stop video output before direct drawing.
    videoDisable();

    currentTest = index;

    const DemoTest& test = tests[currentTest];

    setDisplayProfile(test.profile);

    if (test.usesVideoScanner)
    {
        test.render();
        videoEnable();
    }

    testStartMillis = millis();
}

static void nextTest()
{
    startTest((currentTest + 1) % TEST_COUNT);
}

// -----------------------------------------------------------------------------
// Setup / loop
// -----------------------------------------------------------------------------

void setup()
{
    pinMode(2, OUTPUT);

    Serial.setRxBufferSize(10100);
    Serial.begin(3000000);

    hardwareInit();

    videoInit();
    videoStart();

    SerialProto.begin(Serial, onFrameReceived);

    xTaskCreatePinnedToCore(
        serialTask,
        "serialRx",
        SERIAL_TASK_STACK,
        nullptr,
        SERIAL_TASK_PRIORITY,
        nullptr,
        1
    );

    clockStartMillis = millis();
    startTest(0);
}

void loop()
{
    const uint32_t now = millis();

    if (showingVideo)
    {
        if (now - lastFrameMillis >= VIDEO_TIMEOUT_MS)
        {
            showingVideo = false;
            videoDisable();

            startTest(currentTest);
        }
        else
        {
            delay(1);
        }

        return;
    }

    const DemoTest& test = tests[currentTest];

    if (now - testStartMillis >= TEST_DURATION_MS)
    {
        nextTest();
        return;
    }

    if (test.usesVideoScanner)
    {
        // The scanner renders the prepared framebuffer.
        delay(1);
        return;
    }

    if (!setDisplayProfile(test.profile))
        return;

    beamOn();
    test.render();
}