#include "SerialProtocol.h"
#include "common.h"
static constexpr int PARSE_BUDGET = 1024;

void SerialProtocol::begin(
    Stream& serial,
    FrameCallback fcb)
{
    _serial = &serial;
    _frameCallback = fcb;
}

void SerialProtocol::update()
{
    int budget = PARSE_BUDGET;

    while (budget-- > 0 && _serial->available())
    {
        const uint8_t b = _serial->read();

        switch (state)
        {
            case RXState::WAIT_HEADER1:

                if (b == HEADER1)
                    state = RXState::WAIT_HEADER2;

                break;


            case RXState::WAIT_HEADER2:

                if (b == HEADER2)
                {
                    state = RXState::LEN_L;
                }
                else if (b == HEADER1)
                {
                    // Useful when 0xAA appears twice in a row.
                    state = RXState::WAIT_HEADER2;
                }
                else
                {
                    state = RXState::WAIT_HEADER1;
                }

                break;


            case RXState::LEN_L:

                len = b;
                state = RXState::LEN_H;

                break;


            
    case RXState::LEN_H:

        len |= static_cast<uint16_t>(b) << 8;

        // Never allow the payload to exceed the receive buffer.
        if (len == 0 || len > sizeof(buffer))
            state = RXState::WAIT_HEADER1;
        else
            state = RXState::MODE;

        break;


        case RXState::MODE:

            mode = b;

            // Mode 2: existing 100x100 2-bit raster frame.
            if (mode == 2)
            {
                if (len != PACKED2_SIZE)
                {
                    state = RXState::WAIT_HEADER1;
                    break;
                }
            }
            // Mode 3: vector lines, four bytes per segment.
            else if (mode == 3)
            {
                if (len % 4 != 0)
                {
                    state = RXState::WAIT_HEADER1;
                    break;
                }
            }
            else
            {
                state = RXState::WAIT_HEADER1;
                break;
            }

            checksum = mode;
            pos = 0;
            state = RXState::DATA;

            break;


    case RXState::DATA:

        buffer[pos++] = b;
        checksum ^= b;

        if (pos == len)
            state = RXState::CHECKSUM;

        break;


    case RXState::CHECKSUM:

        if (checksum == b)
        {
            if (_frameCallback)
            {
                _frameCallback(buffer, len, mode);
                _serial->write('k');
            }
        }

        state = RXState::WAIT_HEADER1;

        break;
        }
    }
}