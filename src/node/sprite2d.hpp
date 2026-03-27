#pragma once
#include "node2d.hpp"
#include "../renderer/texture.hpp"
#include "../color/color.hpp"
#include "../window/window.hpp"

class Sprite2D : public Node2D {
public:
    Texture texture = {0, 0, 0};
    float width = 64, height = 64;
    Color tint = WHITE;

    Sprite2D(const std::string& name = "Sprite2D") : Node2D(name) {}

    void SetTexture(Texture& tex) {
        texture = tex;
        width = tex.width;
        height = tex.height;
    }

    void Draw() override {
        if (texture.id != 0)
            DrawTexture(texture, x, y, width, height, tint);
        else
            DrawRectangle(x, y, width, height, tint);
    }
};
