#pragma once

#include <glm/glm.hpp>
#include <GL/glad.h>
#include "renderer.hpp"

// Forward declaration
struct Sprite;

class Renderer2D {
private:
	GLuint rectangleVAO, rectangleVBO;
	GLuint circleVAO, circleVBO;
	GLuint shaderProgram;
	
	// Math utilities
	glm::mat4 orthoMatrix;
	
public:
	void Init(int screenWidth, int screenHeight);
	void DrawRectangle(float x, float y, float w, float h, glm::vec4 color);
	void DrawCircle(float x, float y, float radius, glm::vec4 color);
	void DrawLine(float x1, float y1, float x2, float y2, glm::vec4 color);
	void DrawSprite(const Sprite& sprite);
};
