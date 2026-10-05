#include "SerialProtocol.h"
void SerialProtocol::begin(Stream& serial, FrameCallback fcb, TextCallback tcb, volatile DisplayMode* modePtr)
{
    _serial = &serial;
    _frameCallback = fcb;
    _textCallback  = tcb;
    _mode = modePtr;
}

void SerialProtocol::update()
{
    while (_serial->available())
    {
        uint8_t b = _serial->read();

        switch (state)
        {
            case RXState::WAIT_HEADER1:
                if (b == HEADER1)
                {
                    state = RXState::WAIT_HEADER2;
                }
                break;

            case RXState::WAIT_HEADER2:
                if (b == HEADER2)
                {
                    state = RXState::LEN_L;
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
                len |= (b << 8);

                // Reject impossible packets.
                if (len > sizeof(buffer))
                {
                    state = RXState::WAIT_HEADER1;
                }
                else
                {
                    state = RXState::MODE;
                }
                break;

            case RXState::MODE:
                mode = b;
                checksum = b;
                pos = 0;
                state = RXState::DATA;
                break;

            case RXState::DATA:
                buffer[pos++] = b;
                checksum ^= b;

                if (pos >= len)
                    state = RXState::CHECKSUM;

                break;

            case RXState::CHECKSUM:
            {
                if (checksum == b && _frameCallback)
                {
                    // ACK immediately after successful reception.
                    _serial->write("k", 1);

                    _frameCallback(buffer, len);
                }

                state = RXState::WAIT_HEADER1;
                break;
            }
        }
    }
}