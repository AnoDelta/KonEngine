#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"
#include "opengl_renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

OpenGLRenderer::OpenGLRenderer() 
	: rectangleVAO(0), rectangleVBO(0), circleVAO(0), circleVBO(0), 
	  lineVAO(0), lineVBO(0), shaderProgram(0),
	  textureVAO(0), textureVBO(0), textureShaderProgram(0) {
}

OpenGLRenderer::~OpenGLRenderer() {
	glDeleteVertexArrays(1, &rectangleVAO);
	glDeleteBuffers(1, &rectangleVBO);
	glDeleteVertexArrays(1, &circleVAO);
	glDeleteBuffers(1, &circleVBO);
	glDeleteVertexArrays(1, &lineVAO);
	glDeleteBuffers(1, &lineVBO);
	glDeleteProgram(shaderProgram);
	glDeleteVertexArrays(1, &textureVAO);
	glDeleteBuffers(1, &textureVBO);
	glDeleteProgram(textureShaderProgram);
}

void OpenGLRenderer::Init() {
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	SetupShaders();
	CreateRectangleBuffers();
	CreateCircleBuffers();
	CreateLineBuffers();
	SetupTextureShader();
	CreateTextureBuffers();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLRenderer::Present() {
}

void OpenGLRenderer::Clear(float r, float g, float b) {
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::SetProjectionMatrix(int screenWidth, int screenHeight) {
	projectionMatrix = glm::ortho(0.0f, (float)screenWidth, 
								  (float)screenHeight, 0.0f, -1.0f, 1.0f);
	
	glUseProgram(shaderProgram);
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glUseProgram(textureShaderProgram);
	GLint texProjLoc = glGetUniformLocation(textureShaderProgram, "projection");
	glUniformMatrix4fv(texProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
}

void OpenGLRenderer::SetupShaders() {
	const char* vertexSource = R"(
		#version 330 core
		layout(location = 0) in vec2 position;
		
		uniform mat4 projection;
		uniform mat4 transform;
		
		void main() {
			gl_Position = projection * transform * vec4(position, 0.0, 1.0);
		}
	)";
	
	const char* fragmentSource = R"(
		#version 330 core
		uniform vec4 color;
		out vec4 FragColor;
		
		void main() {
			FragColor = color;
		}
	)";
	
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexSource, nullptr);
	glCompileShader(vertex);
	
	int success;
	char infoLog[512];
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
		std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
	}
	
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentSource, nullptr);
	glCompileShader(fragment);
	
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
		std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
	}
	
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertex);
	glAttachShader(shaderProgram, fragment);
	glLinkProgram(shaderProgram);
	
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cerr << "Shader program linking failed: " << infoLog << std::endl;
	}
	
	glDeleteShader(vertex);
	glDeleteShader(fragment);
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
		void main() {
			FragColor = texture(tex, TexCoord) * tint;
		}
	)";

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);

	int success; char infoLog[512];
	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vert, 512, nullptr, infoLog);
		std::cerr << "Texture vertex shader failed: " << infoLog << std::endl;
	}

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);

	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(frag, 512, nullptr, infoLog);
		std::cerr << "Texture fragment shader failed: " << infoLog << std::endl;
	}

	textureShaderProgram = glCreateProgram();
	glAttachShader(textureShaderProgram, vert);
	glAttachShader(textureShaderProgram, frag);
	glLinkProgram(textureShaderProgram);

	glDeleteShader(vert);
	glDeleteShader(frag);
}

void OpenGLRenderer::CreateRectangleBuffers() {
	float vertices[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	
	glGenVertexArrays(1, &rectangleVAO);
	glGenBuffers(1, &rectangleVBO);
	
	glBindVertexArray(rectangleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::CreateCircleBuffers() {
	const int segments = 36;
	float vertices[segments * 2 + 4];
	
	vertices[0] = 0.0f;
	vertices[1] = 0.0f;
	
	for (int i = 0; i <= segments; i++) {
		float angle = 2.0f * 3.14159265f * i / segments;
		vertices[2 + i * 2] = 0.5f + 0.5f * cos(angle);
		vertices[3 + i * 2] = 0.5f + 0.5f * sin(angle);
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
	float vertices[] = {
		0.0f, 0.0f,
		1.0f, 1.0f
	};
	
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

	// position (location 0)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texcoord (location 1)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::DrawRectangle(float x, float y, float width, float height, 
								   float r, float g, float b, float a) {
	glUseProgram(shaderProgram);
	
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
	transform = glm::scale(transform, glm::vec3(width, height, 1.0f));
	
	GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
	
	GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
	glUniform4f(colorLoc, r, g, b, a);
	
	glBindVertexArray(rectangleVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void OpenGLRenderer::DrawCircle(float x, float y, float radius, 
								float r, float g, float b, float a) {
	glUseProgram(shaderProgram);
	
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
	transform = glm::scale(transform, glm::vec3(radius * 2, radius * 2, 1.0f));
	
	GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
	
	GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
	glUniform4f(colorLoc, r, g, b, a);
	
	glBindVertexArray(circleVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 38);
}

void OpenGLRenderer::DrawLine(float x1, float y1, float x2, float y2, 
							   float r, float g, float b, float a) {
	glUseProgram(shaderProgram);
	
	glm::mat4 transform = glm::mat4(1.0f);
	
	GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
	
	GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
	glUniform4f(colorLoc, r, g, b, a);
	
	float vertices[] = { x1, y1, x2, y2 };
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	
	glBindVertexArray(lineVAO);
	glLineWidth(2.0f);
	glDrawArrays(GL_LINE_STRIP, 0, 2);
	glLineWidth(1.0f);
}

unsigned int OpenGLRenderer::LoadTexture(const char* path) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_set_flip_vertically_on_load(true);
	int w, h, channels;
	unsigned char* data = stbi_load(path, &w, &h, &channels, 0);
	if (!data) {
		std::cerr << "Failed to load texture: " << path << std::endl;
		glDeleteTextures(1, &id);
		return 0;
	}

	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
	return id;
}

void OpenGLRenderer::UnloadTexture(unsigned int id) {
	glDeleteTextures(1, &id);
}

void OpenGLRenderer::DrawTexture(unsigned int id, float x, float y,
								  float width, float height) {
	DrawTextureRec(id, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f);
}

void OpenGLRenderer::DrawTextureRec(unsigned int id, float x, float y,
									 float width, float height,
									 float srcX, float srcY,
									 float srcWidth, float srcHeight) {
	glUseProgram(textureShaderProgram);

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
	transform = glm::scale(transform, glm::vec3(width, height, 1.0f));

	glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "projection"),
					   1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "transform"),
					   1, GL_FALSE, glm::value_ptr(transform));
	glUniform4f(glGetUniformLocation(textureShaderProgram, "tint"), 1, 1, 1, 1);

	float verts[] = {
		0.0f, 0.0f,  srcX,            srcY + srcHeight,
		1.0f, 0.0f,  srcX + srcWidth, srcY + srcHeight,
		1.0f, 1.0f,  srcX + srcWidth, srcY,
		0.0f, 1.0f,  srcX,            srcY,
	};

	glBindVertexArray(textureVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);
	glUniform1i(glGetUniformLocation(textureShaderProgram, "tex"), 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void OpenGLRenderer::DrawRectangle(float x, float y, float width, float height, Color color) {
	DrawRectangle(x, y, width, height, color.r, color.g, color.b, color.a);
}
void OpenGLRenderer::DrawCircle(float x, float y, float radius, Color color) {
	DrawCircle(x, y, radius, color.r, color.g, color.b, color.a);
}
void OpenGLRenderer::DrawLine(float x1, float y1, float x2, float y2, Color color) {
	DrawLine(x1, y1, x2, y2, color.r, color.g, color.b, color.a);
}

void OpenGLRenderer::DrawTexture(unsigned int id, float x, float y, float width, float height, Color tint) {
	DrawTextureRec(id, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, tint);
}
void OpenGLRenderer::DrawTextureRec(unsigned int id, float x, float y, float width, float height,
									float srcX, float srcY, float srcWidth, float srcHeight, Color tint) {
	glUseProgram(textureShaderProgram);

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
	transform = glm::scale(transform, glm::vec3(width, height, 1.0f));

	glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "projection"),
					   1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "transform"),
					   1, GL_FALSE, glm::value_ptr(transform));
	glUniform4f(glGetUniformLocation(textureShaderProgram, "tint"), tint.r, tint.g, tint.b, tint.a);

	float verts[] = {
		0.0f, 0.0f,  srcX,            srcY + srcHeight,
		1.0f, 0.0f,  srcX + srcWidth, srcY + srcHeight,
		1.0f, 1.0f,  srcX + srcWidth, srcY,
		0.0f, 1.0f,  srcX,            srcY,
	};

	glBindVertexArray(textureVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);
	glUniform1i(glGetUniformLocation(textureShaderProgram, "tex"), 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									}
