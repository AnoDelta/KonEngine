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
        float dx = DrawX(width);
        float dy = DrawY(height);

        if (texture.id != 0) {
            if (useSourceRect && texture.width > 0 && texture.height > 0) {
                // Convert pixel src rect to UV (0.0-1.0) — that's what the renderer expects
                float u0 = srcX                  / (float)texture.width;
                float v0 = srcY                  / (float)texture.height;
                float u1 = (srcX + srcWidth)     / (float)texture.width;
                float v1 = (srcY + srcHeight)    / (float)texture.height;
                DrawTextureRec(texture, dx, dy, width, height, u0, v0, u1, v1, tint);
            } else {
                DrawTexture(texture, dx, dy, width, height, tint);
            }
        } else {
            DrawRectangle(dx, dy, width, height, tint);
        }
    }
};
