#pragma once

#include "../color/color.hpp"

struct GlyphInfo {
    float u0, v0, u1, v1;  // UV coords in atlas
    float offsetX, offsetY; // drawing offset
    float advanceX;         // how far to move cursor after drawing
    int width, height;      // glyph size in pixels
};

struct Font {
    unsigned int atlasID;           // GPU texture
    GlyphInfo glyphs[128];          // ASCII only
    int fontSize;
    float lineHeight;
};

Font LoadFont(const char* path, int fontSize);
Font LoadDefaultFont(int fontSize);
Font& GetDefaultFont();
void UnloadFont(Font& font);

void DrawText(Font& font, const char* text, float x, float y, Color color);
void DrawText(Font& font, const char* text, float x, float y, int fontSize, Color color);

// Uses default font
void DrawText(const char* text, float x, float y, Color color);
void DrawText(const char* text, float x, float y, int fontSize, Color color);
