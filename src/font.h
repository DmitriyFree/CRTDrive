#pragma once
#include <stdint.h>
#include <stddef.h>

struct Glyph
{
    char character;
    uint8_t rows[7];
};

extern const Glyph FONT[];
extern const size_t FONT_COUNT;

const Glyph* findGlyph(char c);