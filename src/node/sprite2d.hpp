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

	// Animation source rect
	float srcX = 0, srcY = 0;
	float srcWidth = 0, srcHeight = 0;
	bool useSourceRect = false;

	Sprite2D(const std::string& name = "Sprite2D") : Node2D(name) {}

	void SetTexture(Texture& tex) {
		texture = tex;
		width = tex.width;
		height = tex.height;
	}

	void Draw() override {
		float dx = DrawX(width);
		float dy = DrawY(height);

		if (texture.id != 0) {
			if (useSourceRect && texture.width > 0 && texture.height > 0) {
				float u0 = srcX              / (float)texture.width;
				float v0 = srcY              / (float)texture.height;
				float u1 = (srcX + srcWidth) / (float)texture.width;   // ← fixed
				float v1 = (srcY + srcHeight)/ (float)texture.height;  // ← fixed
				DrawTextureRec(texture, dx, dy, width, height, u0, v0, u1, v1, tint);
			} else {
				DrawTexture(texture, dx, dy, width, height, tint);
			}
		} else {
			DrawRectangle(dx, dy, width, height, tint);
		}
	}
};
