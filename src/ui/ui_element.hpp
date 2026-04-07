#pragma once

#include "../color/color.hpp"
#include "../font/font.hpp"
#include "../window/window.hpp"
#include "../renderer/texture.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
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

    // --- Signal system ---
    void Connect(const std::string& signal, std::function<void()> cb) {
        signals[signal].push_back(cb);
    }
    void Emit(const std::string& signal) {
        auto it = signals.find(signal);
        if (it != signals.end())
            for (auto& cb : it->second) cb();
    }

protected:
    std::unordered_map<std::string, std::vector<std::function<void()>>> signals;
};

// ---------------------------------------------------------------------------
// UIImage -- displays a texture at a position
// ---------------------------------------------------------------------------
class UIImage : public UIElement {
public:
    Texture texture = {0, 0, 0};
    Color tint = WHITE;

    UIImage(const std::string& id) : UIElement(id) {}

    void SetTexture(Texture tex) {
        texture = tex;
        // Auto-size to texture if dimensions not set
        if (width <= 0) width = (float)tex.width;
        if (height <= 0) height = (float)tex.height;
    }

    void Draw() override {
        if (!visible) return;
        if (texture.id != 0) {
            DrawTexture(texture, x, y, width, height, tint);
        } else {
            DrawRectangle(x, y, width, height, tint);
        }
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
// Supports optional icon texture displayed left of the text.
//
// Signals:
//   "clicked"  -- fired when the button is clicked
//   "hovered"  -- fired when mouse enters the button
//   "unhovered" -- fired when mouse leaves the button
// ---------------------------------------------------------------------------
class UIButton : public UIElement {
public:
    std::string text;
    int fontSize = 18;

    // Optional icon texture (displayed left of text)
    Texture icon = {0, 0, 0};
    float iconW = 0, iconH = 0;   // icon display size (0 = auto from texture)
    float iconPadding = 6.0f;     // space between icon and text

    // Optional background texture (replaces solid color fill)
    Texture background = {0, 0, 0};
    Texture backgroundHover = {0, 0, 0};
    Texture backgroundPressed = {0, 0, 0};

    Color normalColor  = Color(0.25f, 0.25f, 0.30f, 1.0f);
    Color hoverColor   = Color(0.35f, 0.35f, 0.42f, 1.0f);
    Color pressedColor = Color(0.15f, 0.15f, 0.20f, 1.0f);
    Color textColor    = WHITE;
    Color borderColor  = Color(0.5f, 0.5f, 0.55f, 1.0f);

    float paddingX = 16.0f;
    float paddingY = 8.0f;

    bool hovered = false;
    bool pressed = false;

    // Direct callback (still works, signals are an alternative)
    std::function<void()> onClick;

    UIButton(const std::string& id) : UIElement(id) {}

    void SetIcon(Texture tex, float w = 0, float h = 0) {
        icon = tex;
        iconW = (w > 0) ? w : (float)tex.width;
        iconH = (h > 0) ? h : (float)tex.height;
    }

    void SetBackground(Texture normal, Texture hover = {0,0,0}, Texture press = {0,0,0}) {
        background = normal;
        backgroundHover = (hover.id != 0) ? hover : normal;
        backgroundPressed = (press.id != 0) ? press : normal;
    }

    void Update(float mouseX, float mouseY, bool mouseDown, bool mousePressed) override {
        if (!visible || !enabled) { hovered = false; pressed = false; return; }

        // Auto-size if dimensions not set
        if (width <= 0) {
            float tw = MeasureTextWidth(text.c_str(), fontSize);
            float iw = (icon.id != 0) ? iconW + iconPadding : 0;
            width = tw + iw + paddingX * 2;
        }
        if (height <= 0) height = (float)fontSize + paddingY * 2;

        bool wasHovered = hovered;
        hovered = Contains(mouseX, mouseY);
        pressed = hovered && mouseDown;

        // Hover signals
        if (hovered && !wasHovered) Emit("hovered");
        if (!hovered && wasHovered) Emit("unhovered");

        if (hovered && mousePressed) {
            if (onClick) onClick();
            Emit("clicked");
        }
    }

    void Draw() override {
        if (!visible) return;

        // Background -- texture or solid color
        if (pressed && backgroundPressed.id != 0) {
            DrawTexture(backgroundPressed, x, y, width, height, WHITE);
        } else if (hovered && backgroundHover.id != 0) {
            DrawTexture(backgroundHover, x, y, width, height, WHITE);
        } else if (background.id != 0) {
            DrawTexture(background, x, y, width, height, WHITE);
        } else {
            Color bg = pressed ? pressedColor : (hovered ? hoverColor : normalColor);
            DrawRectangle(x, y, width, height, bg);
        }

        // Border (skip if using background textures)
        if (background.id == 0) {
            DrawLine(x, y, x + width, y, borderColor);
            DrawLine(x, y + height, x + width, y + height, borderColor);
            DrawLine(x, y, x, y + height, borderColor);
            DrawLine(x + width, y, x + width, y + height, borderColor);
        }

        // Icon + centered text
        float tw = MeasureTextWidth(text.c_str(), fontSize);
        float iw = (icon.id != 0) ? iconW + iconPadding : 0;
        float totalW = tw + iw;
        float startX = x + (width - totalW) * 0.5f;

        if (icon.id != 0) {
            float iy = y + (height - iconH) * 0.5f;
            DrawTexture(icon, startX, iy, iconW, iconH, WHITE);
            startX += iconW + iconPadding;
        }

        float ty = y + (height - (float)fontSize) * 0.5f;
        DrawText(text.c_str(), startX, ty, fontSize, textColor);
    }
};

// ---------------------------------------------------------------------------
// UIPanel -- colored rectangle container with optional background texture
// ---------------------------------------------------------------------------
class UIPanel : public UIElement {
public:
    Color backgroundColor = Color(0.1f, 0.1f, 0.15f, 0.85f);
    Color borderColor     = Color(0.4f, 0.4f, 0.45f, 1.0f);

    // Optional background texture (replaces solid color fill)
    Texture background = {0, 0, 0};

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
        if (background.id != 0) {
            DrawTexture(background, x, y, width, height, WHITE);
        } else {
            DrawRectangle(x, y, width, height, backgroundColor);
        }

        if (background.id == 0) {
            DrawLine(x, y, x + width, y, borderColor);
            DrawLine(x, y + height, x + width, y + height, borderColor);
            DrawLine(x, y, x, y + height, borderColor);
            DrawLine(x + width, y, x + width, y + height, borderColor);
        }
    }
};
