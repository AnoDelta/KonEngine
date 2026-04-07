#pragma once

#include "../color/color.hpp"
#include "../font/font.hpp"
#include "../window/window.hpp"
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

// ---------------------------------------------------------------------------
// UIElement -- base class for all screen-space UI elements
// Positioned in design-resolution coordinates (not affected by camera).
// ---------------------------------------------------------------------------
class UIElement {
public:
    std::string id;
    float x = 0, y = 0;
    float width = 0, height = 0;
    bool visible = true;
    bool enabled = true;
    int zOrder = 0;

    UIElement(const std::string& id) : id(id) {}
    virtual ~UIElement() = default;

    virtual void Update(float mouseX, float mouseY, bool mouseDown, bool mousePressed) {}
    virtual void Draw() {}

    bool Contains(float px, float py) const {
        return visible && px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

// ---------------------------------------------------------------------------
// UILabel -- static text display
// ---------------------------------------------------------------------------
class UILabel : public UIElement {
public:
    std::string text;
    int fontSize = 20;
    Color color = WHITE;

    UILabel(const std::string& id) : UIElement(id) {}

    void Draw() override {
        if (!visible) return;
        DrawText(text.c_str(), x, y, fontSize, color);
    }

    float TextWidth() const {
        Font& f = GetDefaultFont();
        return MeasureTextWidth(f, text.c_str());
    }
};

// ---------------------------------------------------------------------------
// UIButton -- clickable rectangle with text, hover/pressed visual states
// ---------------------------------------------------------------------------
class UIButton : public UIElement {
public:
    std::string text;
    int fontSize = 18;

    Color normalColor  = Color(0.25f, 0.25f, 0.30f, 1.0f);
    Color hoverColor   = Color(0.35f, 0.35f, 0.42f, 1.0f);
    Color pressedColor = Color(0.15f, 0.15f, 0.20f, 1.0f);
    Color textColor    = WHITE;
    Color borderColor  = Color(0.5f, 0.5f, 0.55f, 1.0f);

    float paddingX = 16.0f;
    float paddingY = 8.0f;

    bool hovered = false;
    bool pressed = false;

    std::function<void()> onClick;

    UIButton(const std::string& id) : UIElement(id) {}

    void Update(float mouseX, float mouseY, bool mouseDown, bool mousePressed) override {
        if (!visible || !enabled) { hovered = false; pressed = false; return; }

        // Auto-size if dimensions not set
        if (width <= 0) width = MeasureTextWidth(text.c_str(), fontSize) + paddingX * 2;
        if (height <= 0) height = (float)fontSize + paddingY * 2;

        hovered = Contains(mouseX, mouseY);
        pressed = hovered && mouseDown;

        if (hovered && mousePressed && onClick) {
            onClick();
        }
    }

    void Draw() override {
        if (!visible) return;

        // Background
        Color bg = pressed ? pressedColor : (hovered ? hoverColor : normalColor);
        DrawRectangle(x, y, width, height, bg);

        // Border (4 lines)
        DrawLine(x, y, x + width, y, borderColor);
        DrawLine(x, y + height, x + width, y + height, borderColor);
        DrawLine(x, y, x, y + height, borderColor);
        DrawLine(x + width, y, x + width, y + height, borderColor);

        // Centered text
        float tw = MeasureTextWidth(text.c_str(), fontSize);
        float tx = x + (width - tw) * 0.5f;
        float ty = y + (height - (float)fontSize) * 0.5f;
        DrawText(text.c_str(), tx, ty, fontSize, textColor);
    }
};

// ---------------------------------------------------------------------------
// UIPanel -- colored rectangle container with optional children
// ---------------------------------------------------------------------------
class UIPanel : public UIElement {
public:
    Color backgroundColor = Color(0.1f, 0.1f, 0.15f, 0.85f);
    Color borderColor     = Color(0.4f, 0.4f, 0.45f, 1.0f);

    // Child element IDs -- children are owned by the UI manager, not the panel.
    // Panel applies its position as an offset to children during update/draw.
    std::vector<std::string> childIds;

    UIPanel(const std::string& id) : UIElement(id) {}

    void AddChild(const std::string& childId) {
        childIds.push_back(childId);
    }

    void RemoveChild(const std::string& childId) {
        childIds.erase(
            std::remove(childIds.begin(), childIds.end(), childId),
            childIds.end());
    }

    void Draw() override {
        if (!visible) return;
        DrawRectangle(x, y, width, height, backgroundColor);

        DrawLine(x, y, x + width, y, borderColor);
        DrawLine(x, y + height, x + width, y + height, borderColor);
        DrawLine(x, y, x, y + height, borderColor);
        DrawLine(x + width, y, x + width, y + height, borderColor);
    }
};
