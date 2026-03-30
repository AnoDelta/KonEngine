#pragma once

#include "../renderer.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../../color/color.hpp"

class OpenGLRenderer : public IRenderer {
private:
	// --- Geometry VAOs/VBOs (circles, lines — not batched) ---
	GLuint circleVAO, circleVBO;
	GLuint lineVAO,   lineVBO;

	// --- Colored shape shader (circles, lines) ---
	GLuint shaderProgram;
	GLint  loc_shape_proj;
	GLint  loc_shape_transform;
	GLint  loc_shape_color;

	// --- Texture shader ---
	GLuint textureVAO, textureVBO;
	GLuint textureShaderProgram;
	GLint  loc_tex_proj;
	GLint  loc_tex_transform;
	GLint  loc_tex_tint;
	GLint  loc_tex_sampler;

	// --- Text/glyph shader ---
	GLuint textVAO, textVBO;
	GLuint textShaderProgram;
	GLint  loc_text_proj;
	GLint  loc_text_color;
	GLint  loc_text_sampler;

	// --- Colored quad batcher (DrawRectangle) ---
	// Vertex layout: x, y, r, g, b, a  (6 floats)
	// Each quad = 6 vertices (2 triangles, pre-expanded in world space)
	static constexpr int MAX_BATCH_QUADS = 8192;
	static constexpr int BATCH_FLOATS_PER_QUAD = 36; // 6 verts * 6 floats
	GLuint batchVAO, batchVBO;
	GLuint batchShaderProgram;
	GLint  loc_batch_proj;
	float  quadBuffer[MAX_BATCH_QUADS * BATCH_FLOATS_PER_QUAD];
	int    quadCount = 0;

	// --- State cache ---
	GLuint activeProgram = 0;

	int screenWidth  = 0;
	int screenHeight = 0;
	glm::mat4 projectionMatrix;
	glm::mat4 savedProjectionMatrix;

	// --- Setup helpers ---
	void SetupShaders();
	void SetupTextureShader();
	void SetupTextShader();
	void SetupBatchShader();

	void CreateCircleBuffers();
	void CreateLineBuffers();
	void CreateTextureBuffers();
	void CreateTextBuffers();
	void CreateBatchBuffers();

	// --- Internal helpers ---
	void UseProgram(GLuint prog);
	void FlushQuads();

public:
	OpenGLRenderer();
	~OpenGLRenderer();

	void Init()    override;
	void Clear(float r, float g, float b) override;
	void Present() override;

	void SetProjectionMatrix(int screenWidth, int screenHeight);

	void DrawRectangle(float x, float y, float width, float height,
	                   float r, float g, float b, float a = 1.0f) override;
	void DrawRectangle(float x, float y, float width, float height, Color color) override;

	void DrawCircle(float x, float y, float radius,
	                float r, float g, float b, float a = 1.0f) override;
	void DrawCircle(float x, float y, float radius, Color color) override;

	void DrawLine(float x1, float y1, float x2, float y2,
	              float r, float g, float b, float a = 1.0f) override;
	void DrawLine(float x1, float y1, float x2, float y2, Color color) override;

	Texture LoadTexture(const char* path) override;
	void    UnloadTexture(Texture& texture) override;

	void DrawTexture(Texture& texture, float x, float y, float width, float height) override;
	void DrawTexture(Texture& texture, float x, float y, float width, float height, Color tint) override;
	void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
	                    float srcX, float srcY, float srcWidth, float srcHeight) override;
	void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
	                    float srcX, float srcY, float srcWidth, float srcHeight, Color tint) override;

	void DrawGlyph(unsigned int atlasID, float x, float y, float w, float h,
	               float u0, float v0, float u1, float v1, Color color) override;

	void SetTextProjection();
	void BeginCamera2D(const Camera2D& cam) override;
	void EndCamera2D() override;
};
