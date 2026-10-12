
#include <Arduino.h>

#include "VideoDriver.h"
#include "drawing.h"
#include "common.h"
#include "hardware.h"
#include "SerialProtocol.h"

SerialProtocol SerialProto;

// -----------------------------------------------------------------------------
// Demo configuration
// -----------------------------------------------------------------------------

constexpr uint32_t VIDEO_TIMEOUT_MS = 1000;
constexpr uint32_t PATTERN_DURATION_MS = 3000;

enum class DemoMode : uint8_t
{
    Idle,
    Video
};

enum class Pattern : uint8_t
{
    Square,
    Circle,
    Triangle,
    Cross,
    Diamond,
    Text,
    Chessboard,
    Count
};

static volatile DemoMode demoMode = DemoMode::Idle;
static volatile uint32_t lastFrameMillis = 0;

static Pattern currentPattern = Pattern::Square;
static uint32_t patternStartMillis = 0;
static bool patternInitialized = false;

// -----------------------------------------------------------------------------
// Frame reception
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

        videoSetRasterMode();
        validFrame = true;
    }

    if (validFrame)
    {
        lastFrameMillis = millis();
        demoMode = DemoMode::Video;

        digitalWrite(2, !digitalRead(2));
    }
}

// -----------------------------------------------------------------------------
// Serial reception task
// -----------------------------------------------------------------------------

static void serialTask(void*)
{
    while (true)
    {
        SerialProto.update();
        vTaskDelay(1);
    }
}

// -----------------------------------------------------------------------------
// Framebuffer test pattern
// -----------------------------------------------------------------------------

static void fillChessboard(int cells)
{
    cells = max(cells, 1);

    for (int y = 0; y < VIDEO_HEIGHT; ++y)
    {
        for (int x = 0; x < VIDEO_WIDTH; ++x)
        {
            const bool lit =
                ((x * cells / VIDEO_WIDTH) +
                 (y * cells / VIDEO_HEIGHT)) & 1;

            unpackedFrame[y * VIDEO_WIDTH + x] =
                lit ? 255 : 0;
        }
    }
}

// -----------------------------------------------------------------------------
// Idle test patterns
// -----------------------------------------------------------------------------

static void beginPattern(Pattern pattern)
{
    currentPattern = pattern;
    patternStartMillis = millis();
    patternInitialized = false;

    // Direct-drawn patterns must not compete with the raster scanner.
    videoDisable();

    if (currentPattern == Pattern::Chessboard)
    {
        fillChessboard(5);
        videoSetRasterMode();
        patternInitialized = true;
    }
}

static void advancePattern()
{
    const uint8_t next =
        (static_cast<uint8_t>(currentPattern) + 1) %
        static_cast<uint8_t>(Pattern::Count);

    beginPattern(static_cast<Pattern>(next));
}

static void renderIdlePattern()
{
    if (millis() - patternStartMillis >= PATTERN_DURATION_MS)
    {
        advancePattern();
        return;
    }
    beamOn();
    switch (currentPattern)
    {
        case Pattern::Square:
            drawSquare(250, DELAY_US);
            break;

        case Pattern::Circle:
            drawCircle(250, DELAY_US);
            break;

        case Pattern::Triangle:
            drawTriangle(250, DELAY_US);
            break;

        case Pattern::Cross:
            drawCross(250, DELAY_US);
            break;

        case Pattern::Diamond:
            drawDiamond(250, DELAY_US);
            break;

        case Pattern::Text:
            drawText("CRT DEMO", -250, -100, 12, DELAY_US);
            break;

        case Pattern::Chessboard:
            // Raster video task renders the framebuffer.
            videoSetRasterMode();
            videoEnable();
            vTaskDelay(1);
            break;

        case Pattern::Count:
           
            beginPattern(Pattern::Square);
            break;
    }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup()
{
    pinMode(2, OUTPUT);

    Serial.setRxBufferSize(10100);
    Serial.begin(3000000);

    hardwareInit();
    videoInit();
    videoStart();
    beamOn();

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

    beginPattern(Pattern::Square);
}

// -----------------------------------------------------------------------------
// Main demo state machine
// -----------------------------------------------------------------------------

void loop()
{
    const uint32_t now = millis();

    if (demoMode == DemoMode::Video)
    {
        if (now - lastFrameMillis >= VIDEO_TIMEOUT_MS)
        {
            demoMode = DemoMode::Idle;
            beginPattern(Pattern::Square);
        }
        else
        {
            delay(1);
            return;
        }
    }

    renderIdlePattern();
}