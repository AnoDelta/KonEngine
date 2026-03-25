#include "opengl_renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

OpenGLRenderer::OpenGLRenderer() 
	: rectangleVAO(0), rectangleVBO(0), circleVAO(0), circleVBO(0), 
	  lineVAO(0), lineVBO(0), shaderProgram(0) {
}

OpenGLRenderer::~OpenGLRenderer() {
	glDeleteVertexArrays(1, &rectangleVAO);
	glDeleteBuffers(1, &rectangleVBO);
	glDeleteVertexArrays(1, &circleVAO);
	glDeleteBuffers(1, &circleVBO);
	glDeleteVertexArrays(1, &lineVAO);
	glDeleteBuffers(1, &lineVBO);
	glDeleteProgram(shaderProgram);
}

void OpenGLRenderer::Init() {
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	SetupShaders();
	CreateRectangleBuffers();
	CreateCircleBuffers();
	CreateLineBuffers();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLRenderer::Present() {
	// Present is handled by window swap
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

void OpenGLRenderer::CreateRectangleBuffers() {
	float vertices[] = {
		0.0f, 0.0f,  // Bottom-left
		1.0f, 0.0f,  // Bottom-right
		1.0f, 1.0f,  // Top-right
		0.0f, 1.0f   // Top-left
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
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

void OpenGLRenderer::DrawTexture(unsigned int textureID, float x, float y, 
								 float width, float height) {
	// TODO: Implement texture rendering with separate shader
}
