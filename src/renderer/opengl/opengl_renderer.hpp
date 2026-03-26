#pragma once

#include "../renderer.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../../color/color.hpp"

class OpenGLRenderer : public IRenderer {
private:
	GLuint rectangleVAO, rectangleVBO;
	GLuint circleVAO, circleVBO;
	GLuint lineVAO, lineVBO;
	GLuint shaderProgram;
	GLuint textureVAO, textureVBO;
	GLuint textureShaderProgram;
	
	glm::mat4 projectionMatrix;
	
	void CreateRectangleBuffers();
	void CreateCircleBuffers();
	void CreateLineBuffers();
	GLuint CompileShaders();
	void SetupShaders();

	void CreateTextureBuffers();
	void SetupTextureShader();

	GLuint textVAO, textVBO;
	GLuint textShaderProgram;

	void CreateTextBuffers();
	void SetupTextShader();
	
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

	unsigned int LoadTexture(const char* path) override;
	void UnloadTexture(unsigned int id) override;
	void DrawTextureRec(unsigned int id, float x, float y,
					 float width, float height,
					 float srcX, float srcY,
					 float srcWidth, float srcHeight) override;

	void DrawRectangle(float x, float y, float width, float height, Color color) override;
	void DrawCircle(float x, float y, float radius, Color color) override;
	void DrawLine(float x1, float y1, float x2, float y2, Color color) override;
	void DrawTexture(unsigned int id, float x, float y, float width, float height, Color tint) override;
	void DrawTextureRec(unsigned int id, float x, float y, float width, float height,
					 float srcX, float srcY, float srcWidth, float srcHeight, Color tint) override;

	void SetTextProjection();

	void DrawGlyph(unsigned int atlasID, float x, float y, float w, float h,
				   float u0, float v0, float u1, float v1, Color color) override;
};
