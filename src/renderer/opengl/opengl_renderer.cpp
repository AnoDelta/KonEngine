#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"
#include "opengl_renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

// -----------------------------------------------------------------------
// Ctor / Dtor
// -----------------------------------------------------------------------

OpenGLRenderer::OpenGLRenderer()
	: screenWidth(0), screenHeight(0),
	  circleVAO(0), circleVBO(0),
	  lineVAO(0),   lineVBO(0),
	  shaderProgram(0),
	  textureVAO(0), textureVBO(0), textureShaderProgram(0),
	  textVAO(0),    textVBO(0),    textShaderProgram(0),
	  batchVAO(0),   batchVBO(0),   batchShaderProgram(0),
	  quadCount(0),  activeProgram(0)
{}

OpenGLRenderer::~OpenGLRenderer() {
	glDeleteVertexArrays(1, &circleVAO);
	glDeleteBuffers(1, &circleVBO);
	glDeleteVertexArrays(1, &lineVAO);
	glDeleteBuffers(1, &lineVBO);
	glDeleteProgram(shaderProgram);

	glDeleteVertexArrays(1, &textureVAO);
	glDeleteBuffers(1, &textureVBO);
	glDeleteProgram(textureShaderProgram);

	glDeleteVertexArrays(1, &textVAO);
	glDeleteBuffers(1, &textVBO);
	glDeleteProgram(textShaderProgram);

	glDeleteVertexArrays(1, &batchVAO);
	glDeleteBuffers(1, &batchVBO);
	glDeleteProgram(batchShaderProgram);
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------

void OpenGLRenderer::Init() {
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	SetupShaders();
	SetupTextureShader();
	SetupTextShader();
	SetupBatchShader();

	CreateCircleBuffers();
	CreateLineBuffers();
	CreateTextureBuffers();
	CreateTextBuffers();
	CreateBatchBuffers();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLRenderer::Present() {
	FlushQuads(); // push any remaining batched rects
}

void OpenGLRenderer::Clear(float r, float g, float b) {
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

void OpenGLRenderer::UseProgram(GLuint prog) {
	if (prog != activeProgram) {
		glUseProgram(prog);
		activeProgram = prog;
	}
}

void OpenGLRenderer::FlushQuads() {
	if (quadCount == 0) return;

	UseProgram(batchShaderProgram);
	glBindVertexArray(batchVAO);
	glBindBuffer(GL_ARRAY_BUFFER, batchVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
	                quadCount * BATCH_FLOATS_PER_QUAD * sizeof(float),
	                quadBuffer);
	glDrawArrays(GL_TRIANGLES, 0, quadCount * 6);
	quadCount = 0;
}

// -----------------------------------------------------------------------
// Projection / camera
// -----------------------------------------------------------------------

void OpenGLRenderer::SetProjectionMatrix(int w, int h) {
	screenWidth  = w;
	screenHeight = h;
	projectionMatrix = glm::ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);

	// Push to all shaders — do it once here, not per draw call
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(loc_shape_proj, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glUseProgram(textureShaderProgram);
	glUniformMatrix4fv(loc_tex_proj,   1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glUseProgram(textShaderProgram);
	glUniformMatrix4fv(loc_text_proj,  1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glUseProgram(batchShaderProgram);
	glUniformMatrix4fv(loc_batch_proj, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	activeProgram = batchShaderProgram;
}

void OpenGLRenderer::BeginCamera2D(const Camera2D& cam) {
	FlushQuads(); // must flush before changing projection

	savedProjectionMatrix = projectionMatrix;

	float hw = screenWidth  / 2.0f;
	float hh = screenHeight / 2.0f;

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(hw, hh, 0.0f));
	view = glm::rotate(view, glm::radians(cam.rotation), glm::vec3(0, 0, 1));
	float zoom = cam.zoom < 0.01f ? 0.01f : cam.zoom;
	view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
	view = glm::translate(view, glm::vec3(-cam.x, -cam.y, 0.0f));

	glm::mat4 camProj = projectionMatrix * view;

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(loc_shape_proj, 1, GL_FALSE, glm::value_ptr(camProj));

	glUseProgram(textureShaderProgram);
	glUniformMatrix4fv(loc_tex_proj, 1, GL_FALSE, glm::value_ptr(camProj));

	glUseProgram(batchShaderProgram);
	glUniformMatrix4fv(loc_batch_proj, 1, GL_FALSE, glm::value_ptr(camProj));

	activeProgram = batchShaderProgram;
}

void OpenGLRenderer::EndCamera2D() {
	FlushQuads(); // flush camera-space rects before restoring projection

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(loc_shape_proj, 1, GL_FALSE, glm::value_ptr(savedProjectionMatrix));

	glUseProgram(textureShaderProgram);
	glUniformMatrix4fv(loc_tex_proj, 1, GL_FALSE, glm::value_ptr(savedProjectionMatrix));

	glUseProgram(batchShaderProgram);
	glUniformMatrix4fv(loc_batch_proj, 1, GL_FALSE, glm::value_ptr(savedProjectionMatrix));

	activeProgram = batchShaderProgram;
}

void OpenGLRenderer::SetTextProjection() {
	// Text always uses screen-space projection — nothing extra needed,
	// SetProjectionMatrix already uploaded it.
}

// -----------------------------------------------------------------------
// Shader setup  (compile → link → cache locations)
// -----------------------------------------------------------------------

void OpenGLRenderer::SetupShaders() {
	const char* vertSrc = R"(
		#version 330 core
		layout(location = 0) in vec2 position;
		uniform mat4 projection;
		uniform mat4 transform;
		void main() {
			gl_Position = projection * transform * vec4(position, 0.0, 1.0);
		}
	)";

	const char* fragSrc = R"(
		#version 330 core
		uniform vec4 color;
		out vec4 FragColor;
		void main() { FragColor = color; }
	)";

	int ok; char log[512];

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "shape vert: " << log << "\n"; }

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "shape frag: " << log << "\n"; }

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vert);
	glAttachShader(shaderProgram, frag);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
	if (!ok) { glGetProgramInfoLog(shaderProgram, 512, nullptr, log); std::cerr << "shape link: " << log << "\n"; }

	glDeleteShader(vert);
	glDeleteShader(frag);

	// Cache locations AFTER linking
	loc_shape_proj      = glGetUniformLocation(shaderProgram, "projection");
	loc_shape_transform = glGetUniformLocation(shaderProgram, "transform");
	loc_shape_color     = glGetUniformLocation(shaderProgram, "color");
}

void OpenGLRenderer::SetupTextureShader() {
	const char* vertSrc = R"(
		#version 330 core
		layout(location = 0) in vec2 position;
		layout(location = 1) in vec2 texCoord;
		uniform mat4 projection;
		uniform mat4 transform;
		out vec2 TexCoord;
		void main() {
			gl_Position = projection * transform * vec4(position, 0.0, 1.0);
			TexCoord = texCoord;
		}
	)";

	const char* fragSrc = R"(
		#version 330 core
		in vec2 TexCoord;
		out vec4 FragColor;
		uniform sampler2D tex;
		uniform vec4 tint;
		void main() { FragColor = texture(tex, TexCoord) * tint; }
	)";

	int ok; char log[512];

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "tex vert: " << log << "\n"; }

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "tex frag: " << log << "\n"; }

	textureShaderProgram = glCreateProgram();
	glAttachShader(textureShaderProgram, vert);
	glAttachShader(textureShaderProgram, frag);
	glLinkProgram(textureShaderProgram);
	glGetProgramiv(textureShaderProgram, GL_LINK_STATUS, &ok);
	if (!ok) { glGetProgramInfoLog(textureShaderProgram, 512, nullptr, log); std::cerr << "tex link: " << log << "\n"; }

	glDeleteShader(vert);
	glDeleteShader(frag);

	loc_tex_proj      = glGetUniformLocation(textureShaderProgram, "projection");
	loc_tex_transform = glGetUniformLocation(textureShaderProgram, "transform");
	loc_tex_tint      = glGetUniformLocation(textureShaderProgram, "tint");
	loc_tex_sampler   = glGetUniformLocation(textureShaderProgram, "tex");
}

void OpenGLRenderer::SetupTextShader() {
	const char* vertSrc = R"(
		#version 330 core
		layout(location = 0) in vec2 position;
		layout(location = 1) in vec2 texCoord;
		uniform mat4 projection;
		out vec2 TexCoord;
		void main() {
			gl_Position = projection * vec4(position, 0.0, 1.0);
			TexCoord = texCoord;
		}
	)";

	const char* fragSrc = R"(
		#version 330 core
		in vec2 TexCoord;
		out vec4 FragColor;
		uniform sampler2D tex;
		uniform vec4 textColor;
		void main() {
			float alpha = texture(tex, TexCoord).r;
			FragColor = vec4(textColor.rgb, textColor.a * alpha);
		}
	)";

	int ok; char log[512];

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "text vert: " << log << "\n"; }

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "text frag: " << log << "\n"; }

	textShaderProgram = glCreateProgram();
	glAttachShader(textShaderProgram, vert);
	glAttachShader(textShaderProgram, frag);
	glLinkProgram(textShaderProgram);
	glGetProgramiv(textShaderProgram, GL_LINK_STATUS, &ok);
	if (!ok) { glGetProgramInfoLog(textShaderProgram, 512, nullptr, log); std::cerr << "text link: " << log << "\n"; }

	glDeleteShader(vert);
	glDeleteShader(frag);

	loc_text_proj    = glGetUniformLocation(textShaderProgram, "projection");
	loc_text_color   = glGetUniformLocation(textShaderProgram, "textColor");
	loc_text_sampler = glGetUniformLocation(textShaderProgram, "tex");
}

void OpenGLRenderer::SetupBatchShader() {
	// Vertex attributes carry world-space position + color directly.
	// No per-draw transform uniform needed — CPU pre-expands quads.
	const char* vertSrc = R"(
		#version 330 core
		layout(location = 0) in vec2 position;
		layout(location = 1) in vec4 color;
		uniform mat4 projection;
		out vec4 vColor;
		void main() {
			gl_Position = projection * vec4(position, 0.0, 1.0);
			vColor = color;
		}
	)";

	const char* fragSrc = R"(
		#version 330 core
		in vec4 vColor;
		out vec4 FragColor;
		void main() { FragColor = vColor; }
	)";

	int ok; char log[512];

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "batch vert: " << log << "\n"; }

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "batch frag: " << log << "\n"; }

	batchShaderProgram = glCreateProgram();
	glAttachShader(batchShaderProgram, vert);
	glAttachShader(batchShaderProgram, frag);
	glLinkProgram(batchShaderProgram);
	glGetProgramiv(batchShaderProgram, GL_LINK_STATUS, &ok);
	if (!ok) { glGetProgramInfoLog(batchShaderProgram, 512, nullptr, log); std::cerr << "batch link: " << log << "\n"; }

	glDeleteShader(vert);
	glDeleteShader(frag);

	loc_batch_proj = glGetUniformLocation(batchShaderProgram, "projection");
}

// -----------------------------------------------------------------------
// Buffer creation
// -----------------------------------------------------------------------

void OpenGLRenderer::CreateCircleBuffers() {
	const int segments = 36;
	float vertices[segments * 2 + 4];
	vertices[0] = 0.5f;
	vertices[1] = 0.5f;
	for (int i = 0; i <= segments; i++) {
		float angle = 2.0f * 3.14159265f * i / segments;
		vertices[2 + i * 2] = 0.5f + 0.5f * cosf(angle);
		vertices[3 + i * 2] = 0.5f + 0.5f * sinf(angle);
	}

	glGenVertexArrays(1, &circleVAO);
	glGenBuffers(1, &circleVBO);
	glBindVertexArray(circleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::CreateLineBuffers() {
	float vertices[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &lineVBO);
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::CreateTextureBuffers() {
	glGenVertexArrays(1, &textureVAO);
	glGenBuffers(1, &textureVBO);
	glBindVertexArray(textureVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::CreateTextBuffers() {
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::CreateBatchBuffers() {
	// Layout: x, y, r, g, b, a  (6 floats per vertex, 6 verts per quad)
	glGenVertexArrays(1, &batchVAO);
	glGenBuffers(1, &batchVBO);
	glBindVertexArray(batchVAO);
	glBindBuffer(GL_ARRAY_BUFFER, batchVBO);
	glBufferData(GL_ARRAY_BUFFER,
	             MAX_BATCH_QUADS * BATCH_FLOATS_PER_QUAD * sizeof(float),
	             nullptr, GL_DYNAMIC_DRAW);
	// location 0: position (xy)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// location 1: color (rgba)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// -----------------------------------------------------------------------
// Draw — Rectangles (batched)
// -----------------------------------------------------------------------

void OpenGLRenderer::DrawRectangle(float x, float y, float w, float h,
                                    float r, float g, float b, float a) {
	if (quadCount >= MAX_BATCH_QUADS)
		FlushQuads();

	float x2 = x + w, y2 = y + h;
	float* v = &quadBuffer[quadCount * BATCH_FLOATS_PER_QUAD];

	// Triangle 1: top-left, top-right, bottom-right
	v[ 0]=x;  v[ 1]=y;  v[ 2]=r; v[ 3]=g; v[ 4]=b; v[ 5]=a;
	v[ 6]=x2; v[ 7]=y;  v[ 8]=r; v[ 9]=g; v[10]=b; v[11]=a;
	v[12]=x2; v[13]=y2; v[14]=r; v[15]=g; v[16]=b; v[17]=a;
	// Triangle 2: top-left, bottom-right, bottom-left
	v[18]=x;  v[19]=y;  v[20]=r; v[21]=g; v[22]=b; v[23]=a;
	v[24]=x2; v[25]=y2; v[26]=r; v[27]=g; v[28]=b; v[29]=a;
	v[30]=x;  v[31]=y2; v[32]=r; v[33]=g; v[34]=b; v[35]=a;

	quadCount++;
}

void OpenGLRenderer::DrawRectangle(float x, float y, float w, float h, Color c) {
	DrawRectangle(x, y, w, h, c.r, c.g, c.b, c.a);
}

// -----------------------------------------------------------------------
// Draw — Circles (not batched, uncommon)
// -----------------------------------------------------------------------

void OpenGLRenderer::DrawCircle(float x, float y, float radius,
                                 float r, float g, float b, float a) {
	FlushQuads(); // flush rects before switching shader

	UseProgram(shaderProgram);

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x - radius, y - radius, 0.0f));
	transform = glm::scale(transform, glm::vec3(radius * 2, radius * 2, 1.0f));

	glUniformMatrix4fv(loc_shape_transform, 1, GL_FALSE, glm::value_ptr(transform));
	glUniform4f(loc_shape_color, r, g, b, a);

	glBindVertexArray(circleVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 38);
}

void OpenGLRenderer::DrawCircle(float x, float y, float radius, Color c) {
	DrawCircle(x, y, radius, c.r, c.g, c.b, c.a);
}

// -----------------------------------------------------------------------
// Draw — Lines (not batched)
// -----------------------------------------------------------------------

void OpenGLRenderer::DrawLine(float x1, float y1, float x2, float y2,
                               float r, float g, float b, float a) {
	FlushQuads();

	UseProgram(shaderProgram);

	glm::mat4 identity = glm::mat4(1.0f);
	glUniformMatrix4fv(loc_shape_transform, 1, GL_FALSE, glm::value_ptr(identity));
	glUniform4f(loc_shape_color, r, g, b, a);

	float verts[] = { x1, y1, x2, y2 };
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glBindVertexArray(lineVAO);
	glLineWidth(2.0f);
	glDrawArrays(GL_LINE_STRIP, 0, 2);
	glLineWidth(1.0f);
}

void OpenGLRenderer::DrawLine(float x1, float y1, float x2, float y2, Color c) {
	DrawLine(x1, y1, x2, y2, c.r, c.g, c.b, c.a);
}

// -----------------------------------------------------------------------
// Textures
// -----------------------------------------------------------------------

Texture OpenGLRenderer::LoadTexture(const char* path) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int w, h, channels;
	// Force 4 channels (RGBA) to avoid GL_RGB row alignment issues
	unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
	if (!data) {
		std::cerr << "Failed to load texture: " << path << "\n";
		glDeleteTextures(1, &id);
		return {0, 0, 0};
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	return {id, w, h};
}

void OpenGLRenderer::UnloadTexture(Texture& texture) {
	glDeleteTextures(1, &texture.id);
	texture = {0, 0, 0};
}

void OpenGLRenderer::DrawTexture(Texture& tex, float x, float y, float w, float h) {
	DrawTextureRec(tex, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, WHITE);
}

void OpenGLRenderer::DrawTexture(Texture& tex, float x, float y, float w, float h, Color tint) {
	DrawTextureRec(tex, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, tint);
}

void OpenGLRenderer::DrawTextureRec(Texture& tex, float x, float y, float w, float h,
                                     float srcX, float srcY, float srcW, float srcH) {
	DrawTextureRec(tex, x, y, w, h, srcX, srcY, srcW, srcH, WHITE);
}

void OpenGLRenderer::DrawTextureRec(Texture& tex, float x, float y, float w, float h,
                                     float srcX, float srcY, float srcW, float srcH, Color tint) {
	FlushQuads(); // flush pending rects before switching shader/texture

	UseProgram(textureShaderProgram);

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
	transform = glm::scale(transform, glm::vec3(w, h, 1.0f));

	glUniformMatrix4fv(loc_tex_transform, 1, GL_FALSE, glm::value_ptr(transform));
	glUniform4f(loc_tex_tint, tint.r, tint.g, tint.b, tint.a);
	glUniform1i(loc_tex_sampler, 0);

	float verts[] = {
		0.0f, 0.0f,  srcX, srcY,
		1.0f, 0.0f,  srcW, srcY,
		1.0f, 1.0f,  srcW, srcH,
		0.0f, 1.0f,  srcX, srcH,
	};

	glBindVertexArray(textureVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex.id);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// -----------------------------------------------------------------------
// Glyphs / text
// -----------------------------------------------------------------------

void OpenGLRenderer::DrawGlyph(unsigned int atlasID,
                                float x, float y, float w, float h,
                                float u0, float v0, float u1, float v1, Color color) {
	FlushQuads();

	UseProgram(textShaderProgram);

	// projection is already uploaded in SetProjectionMatrix — no need to re-upload every glyph
	glUniform4f(loc_text_color, color.r, color.g, color.b, color.a);
	glUniform1i(loc_text_sampler, 0);

	float verts[] = {
		x,     y,      u0, v0,
		x + w, y,      u1, v0,
		x + w, y + h,  u1, v1,
		x,     y + h,  u0, v1,
	};

	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlasID);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
