#pragma once

#include <glm/glm.hpp>
#include <cstdint>

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

	virtual unsigned int LoadTexture(const char* path) = 0;
	virtual void UnloadTexture(unsigned int id) = 0;
	virtual void DrawTexture(unsigned int id, float x, float y, 
						  float width, float height) = 0;
	virtual void DrawTextureRec(unsigned int id, float x, float y,
							 float width, float height,
							 float srcX, float srcY,
							 float srcWidth, float srcHeight) = 0;
};
