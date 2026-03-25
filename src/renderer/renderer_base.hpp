#pragma once

#include <glm/glm.hpp>

// Base for both 2D and 3D
class IRenderer {
public:
	virtual ~IRenderer() = default;
	
	// Shared
	virtual void Init() = 0;
	virtual void Present() = 0;
	virtual void Clear(float r, float g, float b) = 0;
	virtual void SetViewMatrix(const glm::mat4&) = 0;
	virtual void SetProjectionMatrix(const glm::mat4&) = 0;
};

// 2D specific
class IRenderer2D : public IRenderer {
public:
	virtual ~IRenderer2D() = default;
	virtual void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) = 0;
	virtual void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f) = 0;
};

// Future 3D implementation would extend differently
class IRenderer3D : public IRenderer {
public:
	virtual ~IRenderer3D() = default;
	virtual void DrawMesh(unsigned int meshID) = 0;
	virtual void DrawModel(unsigned int modelID) = 0;
};
