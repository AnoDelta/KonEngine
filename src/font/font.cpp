#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "font.hpp"
#include "../asset_manager.hpp"
#include <glad/glad.h>
#include <fstream>
#include <vector>
#include <iostream>
#include "../window/window.hpp"
#include <cstdint>

static Font defaultFont;
static bool defaultFontLoaded = false;

static Font LoadFontFromMemory(const unsigned char* data, int dataSize, int fontSize) {
    Font font;
    font.fontSize = fontSize;

    const int ATLAS_WIDTH  = 512;
    const int ATLAS_HEIGHT = 512;

    stbtt_fontinfo info;
    stbtt_InitFont(&info, data, 0);

    float scale = stbtt_ScaleForPixelHeight(&info, (float)fontSize);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    font.lineHeight = (ascent - descent + lineGap) * scale;

    std::vector<unsigned char> atlas(ATLAS_WIDTH * ATLAS_HEIGHT, 0);
    stbtt_bakedchar bakedChars[96];

    stbtt_BakeFontBitmap(data, 0, (float)fontSize,
                         atlas.data(), ATLAS_WIDTH, ATLAS_HEIGHT,
                         32, 96, bakedChars);

    glGenTextures(1, &font.atlasID);
    glBindTexture(GL_TEXTURE_2D, font.atlasID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 ATLAS_WIDTH, ATLAS_HEIGHT, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (int i = 0; i < 96; i++) {
        stbtt_bakedchar& bc = bakedChars[i];
        GlyphInfo& g = font.glyphs[i + 32];
        g.u0       = bc.x0 / (float)ATLAS_WIDTH;
        g.v0       = bc.y0 / (float)ATLAS_HEIGHT;
        g.u1       = bc.x1 / (float)ATLAS_WIDTH;
        g.v1       = bc.y1 / (float)ATLAS_HEIGHT;
        g.offsetX  = bc.xoff;
        g.offsetY  = bc.yoff;
        g.advanceX = bc.xadvance;
        g.width    = bc.x1 - bc.x0;
        g.height   = bc.y1 - bc.y0;
    }

    return font;
}

Font LoadFont(const char* path, int fontSize) {
    // Route through AssetManager — works with loose files OR .konpak
    auto bytes = AssetManager::readFile(path);
    if (bytes.empty()) {
        std::cerr << "Failed to load font: " << path << "\n";
        return LoadDefaultFont(fontSize);
    }
    return LoadFontFromMemory(bytes.data(), (int)bytes.size(), fontSize);
}

Font LoadDefaultFont(int fontSize) {
    #include "default_font.hpp"
    return LoadFontFromMemory(DEFAULT_FONT_DATA, sizeof(DEFAULT_FONT_DATA), fontSize);
}

Font& GetDefaultFont() {
    if (!defaultFontLoaded) {
        defaultFont = LoadDefaultFont(20);
        defaultFontLoaded = true;
    }
    return defaultFont;
}

void UnloadFont(Font& font) {
    glDeleteTextures(1, &font.atlasID);
    font.atlasID = 0;
}

void DrawText(Font& font, const char* text, float x, float y, Color color) {
    float cursorX = x;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = text[i];
        if (c < 32 || c > 127) continue;
        GlyphInfo& g = font.glyphs[c];
        float gx = cursorX + g.offsetX;
        float gy = y + g.offsetY + font.fontSize;
        DrawGlyph(font.atlasID, gx, gy, g.width, g.height,
                  g.u0, g.v0, g.u1, g.v1, color);
        cursorX += g.advanceX;
    }
}

void DrawText(Font& font, const char* text, float x, float y, int fontSize, Color color) {
    DrawText(font, text, x, y, color);
}

void DrawText(const char* text, float x, float y, Color color) {
    DrawText(GetDefaultFont(), text, x, y, color);
}

void DrawText(const char* text, float x, float y, int fontSize, Color color) {
    DrawText(GetDefaultFont(), text, x, y, fontSize, color);
}

float MeasureTextWidth(Font& font, const char* text) {
    float width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = text[i];
        if (c < 32 || c > 127) continue;
        width += font.glyphs[c].advanceX;
    }
    return width;
}

float MeasureTextWidth(const char* text, int fontSize) {
    return MeasureTextWidth(GetDefaultFont(), text);
}
