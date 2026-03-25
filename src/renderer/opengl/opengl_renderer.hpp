#pragma once

#include "../renderer.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>

class OpenGLRenderer : public IRenderer {
private:
	GLuint rectangleVAO, rectangleVBO;
	GLuint circleVAO, circleVBO;
	GLuint lineVAO, lineVBO;
	GLuint shaderProgram;
	
	glm::mat4 projectionMatrix;
	
	void CreateRectangleBuffers();
	void CreateCircleBuffers();
	void CreateLineBuffers();
	GLuint CompileShaders();
	void SetupShaders();
	
public:
	OpenGLRenderer();
	~OpenGLRenderer();
	
	void Init() override;
	void Clear(float r, float g, float b) override;
	void Present() override;
	
	void DrawRectangle(float x, float y, float width, float height, 
					   float r, float g, float b, float a = 1.0f) override;
	void DrawCircle(float x, float y, float radius, 
					float r, float g, float b, float a = 1.0f) override;
	void DrawLine(float x1, float y1, float x2, float y2, 
				  float r, float g, float b, float a = 1.0f) override;
	void DrawTexture(unsigned int textureID, float x, float y, 
					 float width, float height) override;
	
	void SetProjectionMatrix(int screenWidth, int screenHeight);
};
