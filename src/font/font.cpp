#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "font.hpp"
#include <glad/glad.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <cstdarg>
#include "../window/window.hpp"

// -----------------------------------------------------------------------
// Atlas dimensions — 512x512 comfortably fits all ASCII at sizes up to ~72px
// -----------------------------------------------------------------------
static constexpr int ATLAS_W = 512;
static constexpr int ATLAS_H = 512;

// -----------------------------------------------------------------------
// Internal: bake a Font from raw TTF bytes at a given size
// -----------------------------------------------------------------------
static Font BakeFont(const unsigned char* data, int fontSize) {
    Font font;
    font.fontSize = fontSize;

    stbtt_fontinfo info;
    stbtt_InitFont(&info, data, 0);

    float scale = stbtt_ScaleForPixelHeight(&info, (float)fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    font.lineHeight = (ascent - descent + lineGap) * scale;

    std::vector<unsigned char> atlas(ATLAS_W * ATLAS_H, 0);
    stbtt_bakedchar bakedChars[96]; // ASCII 32..127
    stbtt_BakeFontBitmap(data, 0, (float)fontSize,
                         atlas.data(), ATLAS_W, ATLAS_H,
                         32, 96, bakedChars);

    glGenTextures(1, &font.atlasID);
    glBindTexture(GL_TEXTURE_2D, font.atlasID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 ATLAS_W, ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (int i = 0; i < 96; i++) {
        stbtt_bakedchar& bc = bakedChars[i];
        GlyphInfo& g = font.glyphs[i + 32];
        g.u0       = bc.x0 / (float)ATLAS_W;
        g.v0       = bc.y0 / (float)ATLAS_H;
        g.u1       = bc.x1 / (float)ATLAS_W;
        g.v1       = bc.y1 / (float)ATLAS_H;
        g.offsetX  = bc.xoff;
        g.offsetY  = bc.yoff;
        g.advanceX = bc.xadvance;
        g.width    = bc.x1 - bc.x0;
        g.height   = bc.y1 - bc.y0;
    }

    return font;
}

// -----------------------------------------------------------------------
// Font cache
// Key: (path, fontSize) — path is "" for the built-in default font
// -----------------------------------------------------------------------
struct FontCacheKey {
    std::string path;
    int         fontSize;
    bool operator==(const FontCacheKey& o) const {
        return fontSize == o.fontSize && path == o.path;
    }
};

struct FontCacheKeyHash {
    std::size_t operator()(const FontCacheKey& k) const {
        std::size_t h1 = std::hash<std::string>{}(k.path);
        std::size_t h2 = std::hash<int>{}(k.fontSize);
        return h1 ^ (h2 << 16);
    }
};

// Cache stores the baked Font plus the raw file bytes so we can rebake at
// new sizes without hitting disk again.
struct CacheEntry {
    Font                     font;
    std::vector<unsigned char> rawBytes; // empty for built-in default
};

static std::unordered_map<FontCacheKey, CacheEntry, FontCacheKeyHash> s_cache;

// The default font raw bytes (embedded via default_font.hpp)
static std::vector<unsigned char> s_defaultRaw;
static bool s_defaultRawLoaded = false;

static const std::vector<unsigned char>& GetDefaultRaw() {
    if (!s_defaultRawLoaded) {
        #include "default_font.hpp"
        s_defaultRaw.assign(DEFAULT_FONT_DATA,
                            DEFAULT_FONT_DATA + sizeof(DEFAULT_FONT_DATA));
        s_defaultRawLoaded = true;
    }
    return s_defaultRaw;
}

// -----------------------------------------------------------------------
// GetDefaultFont(fontSize) — cached, rebakes on first request per size
// -----------------------------------------------------------------------
Font& GetDefaultFont(int fontSize) {
    FontCacheKey key{"", fontSize};
    auto it = s_cache.find(key);
    if (it != s_cache.end())
        return it->second.font;

    CacheEntry entry;
    entry.font = BakeFont(GetDefaultRaw().data(), fontSize);
    s_cache[key] = std::move(entry);
    return s_cache[key].font;
}

// -----------------------------------------------------------------------
// GetDefaultFont() — size-20 convenience, same cache
// -----------------------------------------------------------------------
static Font* s_defaultFont20 = nullptr;

Font& GetDefaultFont() {
    if (!s_defaultFont20)
        s_defaultFont20 = &GetDefaultFont(20);
    return *s_defaultFont20;
}

// -----------------------------------------------------------------------
// GetCachedFont(path, fontSize) — loads file on first use, rebakes per size
// -----------------------------------------------------------------------
Font& GetCachedFont(const char* path, int fontSize) {
    FontCacheKey key{path, fontSize};
    auto it = s_cache.find(key);
    if (it != s_cache.end())
        return it->second.font;

    // Load raw bytes if we haven't seen this path at any size
    // Reuse bytes from another size entry if available
    const unsigned char* rawData = nullptr;
    std::vector<unsigned char> freshBytes;

    for (auto& [k, v] : s_cache) {
        if (k.path == path && !v.rawBytes.empty()) {
            rawData = v.rawBytes.data();
            break;
        }
    }

    if (!rawData) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "GetCachedFont: cannot open '" << path
                      << "', falling back to default\n";
            return GetDefaultFont(fontSize);
        }
        std::streamsize sz = file.tellg();
        file.seekg(0);
        freshBytes.resize(sz);
        file.read((char*)freshBytes.data(), sz);
        rawData = freshBytes.data();
    }

    CacheEntry entry;
    entry.rawBytes = freshBytes; // may be empty if we reused
    entry.font = BakeFont(rawData, fontSize);
    s_cache[key] = std::move(entry);
    return s_cache[key].font;
}

// -----------------------------------------------------------------------
// UnloadFontCache — release all GPU atlases
// -----------------------------------------------------------------------
void UnloadFontCache() {
    for (auto& [key, entry] : s_cache) {
        if (entry.font.atlasID)
            glDeleteTextures(1, &entry.font.atlasID);
    }
    s_cache.clear();
    s_defaultFont20  = nullptr;
    s_defaultRawLoaded = false;
    s_defaultRaw.clear();
}

// -----------------------------------------------------------------------
// Manual font management (unchanged API)
// -----------------------------------------------------------------------
Font LoadDefaultFont(int fontSize) {
    return BakeFont(GetDefaultRaw().data(), fontSize);
}

Font LoadFont(const char* path, int fontSize) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "LoadFont: cannot open '" << path << "'\n";
        return LoadDefaultFont(fontSize);
    }
    std::streamsize sz = file.tellg();
    file.seekg(0);
    std::vector<unsigned char> buf(sz);
    file.read((char*)buf.data(), sz);
    return BakeFont(buf.data(), fontSize);
}

void UnloadFont(Font& font) {
    if (font.atlasID) {
        glDeleteTextures(1, &font.atlasID);
        font.atlasID = 0;
    }
}

// -----------------------------------------------------------------------
// Core draw — explicit Font
// -----------------------------------------------------------------------
void DrawText(Font& font, const char* text, float x, float y, Color color) {
    float cursorX = x;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 127) continue;
        GlyphInfo& g = font.glyphs[c];
        float gx = cursorX + g.offsetX;
        float gy = y + g.offsetY + font.fontSize;
        DrawGlyph(font.atlasID, gx, gy, g.width, g.height,
                  g.u0, g.v0, g.u1, g.v1, color);
        cursorX += g.advanceX;
    }
}

// -----------------------------------------------------------------------
// DrawText — default font overloads
// -----------------------------------------------------------------------
void DrawText(const char* text, float x, float y, Color color) {
    DrawText(GetDefaultFont(), text, x, y, color);
}

// fontSize now actually works — fetches the right atlas from the cache
void DrawText(const char* text, float x, float y, int fontSize, Color color) {
    DrawText(GetDefaultFont(fontSize), text, x, y, color);
}

// -----------------------------------------------------------------------
// DrawTextF — printf-style formatted text
// -----------------------------------------------------------------------
static void DrawTextFormatted(Font& font, float x, float y,
                               Color color, const char* fmt, va_list args) {
    // Format into a stack buffer; fall back to heap if too long
    char  stackBuf[512];
    char* buf = stackBuf;

    va_list args2;
    va_copy(args2, args);
    int needed = vsnprintf(stackBuf, sizeof(stackBuf), fmt, args);
    va_end(args2);

    std::vector<char> heapBuf;
    if (needed >= (int)sizeof(stackBuf)) {
        heapBuf.resize(needed + 1);
        vsnprintf(heapBuf.data(), heapBuf.size(), fmt, args);
        buf = heapBuf.data();
    }

    DrawText(font, buf, x, y, color);
}

void DrawTextF(float x, float y, Color color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    DrawTextFormatted(GetDefaultFont(), x, y, color, fmt, args);
    va_end(args);
}

void DrawTextF(float x, float y, int fontSize, Color color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    DrawTextFormatted(GetDefaultFont(fontSize), x, y, color, fmt, args);
    va_end(args);
}

void DrawTextF(Font& font, float x, float y, Color color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    DrawTextFormatted(font, x, y, color, fmt, args);
    va_end(args);
}
