#pragma once

struct Color {
    float r, g, b, a;
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

// Presets
static const Color RED     = {1.0f, 0.0f, 0.0f, 1.0f};
static const Color GREEN   = {0.0f, 1.0f, 0.0f, 1.0f};
static const Color BLUE    = {0.0f, 0.0f, 1.0f, 1.0f};
static const Color WHITE   = {1.0f, 1.0f, 1.0f, 1.0f};
static const Color BLACK   = {0.0f, 0.0f, 0.0f, 1.0f};
static const Color YELLOW  = {1.0f, 1.0f, 0.0f, 1.0f};
static const Color CYAN    = {0.0f, 1.0f, 1.0f, 1.0f};
static const Color MAGENTA = {1.0f, 0.0f, 1.0f, 1.0f};
static const Color ORANGE  = {1.0f, 0.5f, 0.0f, 1.0f};
static const Color GRAY    = {0.5f, 0.5f, 0.5f, 1.0f};
static const Color BLANK   = {0.0f, 0.0f, 0.0f, 0.0f};
