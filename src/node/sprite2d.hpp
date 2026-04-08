#pragma once
#include "node2d.hpp"
#include "../renderer/texture.hpp"
#include "../color/color.hpp"
#include "../window/window.hpp"

class Sprite2D : public Node2D {
public:
    Texture texture  = {0, 0, 0};
    float   width    = 64, height = 64;
    Color   tint     = WHITE;

    // When true, SetTexture resizes the sprite to match the texture's native size.
    // When false (default), the sprite keeps its current width/height.
    bool autoResize = false;

    // Animation overlay — multiplied into tint alpha during rendering only
    float animAlpha = 1.0f;

    // Source rect in PIXEL coords — set by AnimationPlayer each frame
    bool  useSourceRect = false;
    float srcX = 0, srcY = 0, srcWidth = 64, srcHeight = 64;

    Sprite2D(const std::string& name = "Sprite2D") : Node2D(name) {}

    void SetTexture(Texture& tex) {
        texture = tex;
        srcWidth  = (float)tex.width;
        srcHeight = (float)tex.height;
        if (autoResize && !useSourceRect) {
            width  = (float)tex.width;
            height = (float)tex.height;
        }
    }

    void Draw() override {
        // Apply animation overlays: offset position, multiply scale
        float drawW = width  * animScaleX;
        float drawH = height * animScaleY;
        float dx = DrawX(drawW) + animOffsetX;
        float dy = DrawY(drawH) + animOffsetY;

        // Combine animation alpha with tint
        Color drawTint = tint;
        drawTint.a *= animAlpha;

        if (texture.id != 0) {
            if (useSourceRect && texture.width > 0 && texture.height > 0) {
                // Convert pixel src rect to UV (0.0-1.0) — that's what the renderer expects
                float u0 = srcX                  / (float)texture.width;
                float v0 = srcY                  / (float)texture.height;
                float u1 = (srcX + srcWidth)     / (float)texture.width;
                float v1 = (srcY + srcHeight)    / (float)texture.height;
                DrawTextureRec(texture, dx, dy, drawW, drawH, u0, v0, u1, v1, drawTint);
            } else {
                DrawTexture(texture, dx, dy, drawW, drawH, drawTint);
            }
        } else {
            DrawRectangle(dx, dy, drawW, drawH, drawTint);
        }
    }
};
