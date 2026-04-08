#pragma once

#include "../color/color.hpp"
#include <cstdarg>
#include <string>

// -----------------------------------------------------------------------
// GlyphInfo -- one character's metrics inside a baked atlas
// -----------------------------------------------------------------------
struct GlyphInfo {
    float u0, v0, u1, v1;  // UV coords in atlas
    float offsetX, offsetY; // drawing offset
    float advanceX;         // cursor advance after drawing
    int   width, height;    // glyph size in pixels
};

// -----------------------------------------------------------------------
// Font -- a single baked atlas at one size.
// Load with LoadFont() or LoadDefaultFont().
// The font cache (see font.cpp) owns atlases for DrawText(fontSize) calls;
// manually loaded Fonts must be freed with UnloadFont().
// -----------------------------------------------------------------------
struct Font {
    unsigned int atlasID  = 0;
    GlyphInfo    glyphs[128] = {};  // ASCII 32-127
    int          fontSize = 0;
    float        lineHeight = 0.0f;
};

// -----------------------------------------------------------------------
// Manual font management
// -----------------------------------------------------------------------
Font  LoadFont(const char* path, int fontSize);
Font  LoadDefaultFont(int fontSize);
void  UnloadFont(Font& font);

// Returns the default font at size 20, loaded on first call.
Font& GetDefaultFont();

// Returns the default font at a specific size, using the internal cache.
// The returned reference is valid until the cache is cleared (UnloadFontCache).
Font& GetDefaultFont(int fontSize);

// Returns a cached font loaded from a file at a specific size.
// The cache owns the atlas — do not call UnloadFont on the returned reference.
Font& GetCachedFont(const char* path, int fontSize);

// Releases all cached font atlases. Call before shutting down.
void UnloadFontCache();

// -----------------------------------------------------------------------
// DrawText -- explicit Font
// -----------------------------------------------------------------------
void DrawText(Font& font, const char* text, float x, float y, Color color);

// -----------------------------------------------------------------------
// DrawText -- default font, fontSize now actually works via cache
// -----------------------------------------------------------------------
void DrawText(const char* text, float x, float y, Color color);
void DrawText(const char* text, float x, float y, int fontSize, Color color);

// std::string overloads (used by KonScript ternary and f-strings)
inline void DrawText(const std::string& text, float x, float y, Color color) {
    DrawText(text.c_str(), x, y, color);
}
inline void DrawText(const std::string& text, float x, float y, int fontSize, Color color) {
    DrawText(text.c_str(), x, y, fontSize, color);
}

// -----------------------------------------------------------------------
// DrawTextF -- printf-style formatted text
// Variants:
//   DrawTextF(x, y, color, fmt, ...)                -- default font, size 20
//   DrawTextF(x, y, fontSize, color, fmt, ...)      -- default font, cached size
//   DrawTextF(font, x, y, color, fmt, ...)          -- explicit Font
// -----------------------------------------------------------------------
void DrawTextF(float x, float y, Color color, const char* fmt, ...);
void DrawTextF(float x, float y, int fontSize, Color color, const char* fmt, ...);
void DrawTextF(Font& font, float x, float y, Color color, const char* fmt, ...);

// -----------------------------------------------------------------------
// MeasureTextWidth -- returns pixel width of text string
// -----------------------------------------------------------------------
float MeasureTextWidth(const char* text, int fontSize);
float MeasureTextWidth(Font& font, const char* text);

// -----------------------------------------------------------------------
// Text alignment
// -----------------------------------------------------------------------
enum class TextAlign { Left, Center, Right };

// Draw text with alignment: x is the anchor point
//   Left:   text starts at x (default behavior)
//   Center: text is centered on x
//   Right:  text ends at x
void DrawTextAligned(const char* text, float x, float y, int fontSize, Color color, TextAlign align);
inline void DrawTextAligned(const std::string& text, float x, float y, int fontSize, Color color, TextAlign align) {
    DrawTextAligned(text.c_str(), x, y, fontSize, color, align);
}

// Shorthand: draw text centered at (x, y)
void DrawTextCentered(const char* text, float x, float y, int fontSize, Color color);
inline void DrawTextCentered(const std::string& text, float x, float y, int fontSize, Color color) {
    DrawTextCentered(text.c_str(), x, y, fontSize, color);
}
