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
    int budget = 512;

    while (_serial->available() && budget-- > 0)
    {
        uint8_t b = _serial->read();

        switch (state)
        {
            // ====================================================
            // IDLE / TEXT MODE HANDLING
            // ====================================================
            case RXState::WAIT_HEADER1:
            {
                // ONLY text is handled here, no detection logic
                if (*_mode == MODE_TEXT)
                {
                    if (b == '\n' || b == '\r')
                    {
                        if (textLine.length() && _textCallback)
                            _textCallback(textLine);

                        textLine = "";
                    }
                    else
                    {
                        // direct UTF-8 stream (no scoring, no filtering)
                        textLine += (char)b;

                        if (textLine.length() > 64)
                        {
                            if (_textCallback)
                                _textCallback(textLine);

                            textLine = "";
                        }
                    }
                    break;
                }

                // GRAPHICS MODE ONLY: look for frame start
                if (b == HEADER1)
                    state = RXState::WAIT_HEADER2;

                break;
            }

            // ====================================================
            case RXState::WAIT_HEADER2:
                if (b == HEADER2)
                    state = RXState::LEN_L;
                else
                    state = RXState::WAIT_HEADER1;
                break;

            // ====================================================
            case RXState::LEN_L:
                len = b;
                state = RXState::LEN_H;
                break;

            case RXState::LEN_H:
                len |= (b << 8);
                state = RXState::MODE;
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
                if (checksum == b && _frameCallback)
                {
                    _frameCallback(buffer, len);
                    _serial->println("k");
                }

                state = RXState::WAIT_HEADER1;
                break;
        }
    }
}