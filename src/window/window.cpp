#include "window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "../renderer/opengl/opengl_renderer.hpp"
#include "../time/time.hpp"

struct Window::Impl {
	GLFWwindow* handle;
	
	Impl(int width, int height, const std::string& title, bool canResize) {
		if (!glfwInit()) {
			std::cerr << "Failed to initialize GLFW" << std::endl;
			handle = nullptr;
			return;
		}

		if (!canResize)
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		else
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!handle) {
			std::cerr << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}

		glfwMakeContextCurrent(handle);
		glfwSwapInterval(1); 
	}

	~Impl() {
		if (handle) {
			glfwDestroyWindow(handle);
			handle = nullptr;
		}
	}
};

Window::Window(int width, int height, const std::string& title, bool canResize) 
	: impl(std::make_unique<Impl>(width, height, title, canResize)),
	  renderer(std::make_unique<OpenGLRenderer>()) {
	renderer->Init();
	static_cast<OpenGLRenderer*>(renderer.get())->SetProjectionMatrix(width, height);
}

Window::~Window() {
	renderer.reset();
	impl.reset();
};

void Window::pollEvents() {
	glfwPollEvents();
}

bool Window::shouldClose() const {
	return impl->handle == nullptr || glfwWindowShouldClose(impl->handle);
}

void Window::swapBuffers() {
	if (impl->handle) {
		glfwSwapBuffers(impl->handle);
	}
}

int Window::getWidth() {
	int width, height;
	glfwGetFramebufferSize(impl->handle, &width, &height);

	return width;
}

int Window::getHeight() {
	int width, height;
	glfwGetFramebufferSize(impl->handle, &width, &height);

	return height;
}

void Window::clearBackground(float r, float g, float b) {
	renderer->Clear(r, g, b);
}

static Window* window = nullptr;

void InitWindow(int width, int height, const std::string &title, bool canResize) {
	window = new Window(width, height, title, canResize);
}

bool WindowShouldClose() {
	return window && window->shouldClose();
}

void Present() {
	if (window) window->swapBuffers();
}

void PollEvents() {
	if (window) {
		TickTime();
		window->pollEvents();
	}
}

void ClearBackground(float r, float g, float b) {
	if (window) window->clearBackground(r,g,b);
}

int GetWindowWidth() {
	if (!window) return 0;

	return window->getWidth();
}

int GetWindowHeight() {
	if (!window) return 0;

	return window->getHeight();
}

void Window::drawRectangle(float x, float y, float w, float h, float r, float g, float b, float a) {
    renderer->DrawRectangle(x, y, w, h, r, g, b, a);
}
void Window::drawCircle(float x, float y, float radius, float r, float g, float b, float a) {
    renderer->DrawCircle(x, y, radius, r, g, b, a);
}
void Window::drawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
    renderer->DrawLine(x1, y1, x2, y2, r, g, b, a);
}

// Global wrappers at the bottom
void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (window) window->drawRectangle(x, y, w, h, r, g, b, a);
}
void DrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
    if (window) window->drawCircle(x, y, radius, r, g, b, a);
}
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
    if (window) window->drawLine(x1, y1, x2, y2, r, g, b, a);
}
