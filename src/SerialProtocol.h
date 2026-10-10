#pragma once
#include <Arduino.h>
#include "common.h"

typedef void (*FrameCallback)(uint8_t*, uint16_t, uint8_t mode);
typedef void (*TextCallback)(const String&);

class SerialProtocol
{
public:
    void begin(Stream& serial, FrameCallback fcb);
    void update();

private:
    Stream* _serial;
    FrameCallback _frameCallback;

    enum class RXState {
        WAIT_HEADER1,
        WAIT_HEADER2,
        LEN_L,
        LEN_H,
        MODE,
        DATA,
        CHECKSUM
    } state = RXState::WAIT_HEADER1;

    static const uint8_t HEADER1 = 0xAA;
    static const uint8_t HEADER2 = 0x55;

    uint16_t len = 0;
    uint8_t  mode = 0;
    uint8_t  checksum = 0;
    uint16_t pos = 0;

    uint8_t buffer[PACKED2_SIZE];

    // NEW — text line buffer
    String textLine;

    static constexpr uint8_t MODE_BIT1 = 0;
    static constexpr uint8_t MODE_RAW8 = 1;
    static constexpr uint8_t MODE_PACKED2 = 2;
};