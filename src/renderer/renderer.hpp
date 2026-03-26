#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include "../color/color.hpp"
#include "texture.hpp"

class IRenderer {
public:
	virtual ~IRenderer() = default;
	virtual void Init() = 0;
	virtual void Present() = 0;
	virtual void Clear(float r, float g, float b) = 0;

	// 2D Drawing Operations
	virtual void DrawRectangle(float x, float y, float width, float height, 
							float r, float g, float b, float a = 1.0f) = 0;
	virtual void DrawCircle(float x, float y, float radius, 
						 float r, float g, float b, float a = 1.0f) = 0;
	virtual void DrawLine(float x1, float y1, float x2, float y2, 
					   float r, float g, float b, float a = 1.0f) = 0;

	virtual Texture LoadTexture(const char* path) = 0;
	virtual void UnloadTexture(Texture& texture) = 0;
	virtual void DrawTexture(Texture& texture, float x, float y, float width, float height) = 0;
	virtual void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
							 float srcX, float srcY, float srcWidth, float srcHeight) = 0;
	virtual void DrawTexture(Texture& texture, float x, float y, float width, float height, Color tint) = 0;
	virtual void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
							 float srcX, float srcY, float srcWidth, float srcHeight, Color tint) = 0;

	// Just added these for making the color process simpler.
	virtual void DrawRectangle(float x, float y, float width, float height, Color color) = 0;
	virtual void DrawCircle(float x, float y, float radius, Color color) = 0;
	virtual void DrawLine(float x1, float y1, float x2, float y2, Color color) = 0;
	virtual void DrawTexture(unsigned int id, float x, float y, float width, float height, Color tint) = 0;
	virtual void DrawTextureRec(unsigned int id, float x, float y, float width, float height,
								float srcX, float srcY, float srcWidth, float srcHeight, Color tint) = 0;

	virtual void DrawGlyph(unsigned int atlasID, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, Color color) = 0;
};
