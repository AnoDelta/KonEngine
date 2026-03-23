#include "window.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "../renderer/opengl/opengl_renderer.hpp"
#include "../time/time.hpp"

struct Window::Impl {
	GLFWwindow* handle;
	
	Impl(int width, int height, const std::string& title) {
		if (!glfwInit()) {
			std::cerr << "Failed to initialize GLFW" << std::endl;
			handle = nullptr;
			return;
		}

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
		}
		glfwTerminate();
	}
};

Window::Window(int width, int height, const std::string& title) 
	: impl(std::make_unique<Impl>(width, height, title)),
	  renderer(std::make_unique<OpenGLRenderer>()) {
	renderer->Init();
}

Window::~Window() = default;

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

void Window::clearBackground(float r, float g, float b) {
	renderer->Clear(r, g, b);
}

static std::unique_ptr<Window> window = nullptr;

void InitWindow(int width, int height, const std::string &title) {
	window = std::make_unique<Window>(width, height, title);
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
