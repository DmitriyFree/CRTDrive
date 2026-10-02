#pragma once

#include <stdint.h>
#include <stddef.h>

struct Glyph
{
    uint32_t character;
    uint8_t rows[7];
};

extern const Glyph FONT[];
extern const size_t FONT_COUNT;

const Glyph* findGlyph(uint32_t c);
uint32_t decodeUTF8(const char*& text);