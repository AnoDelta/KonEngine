#include "window.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

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
	}

	~Impl() {
		if (handle) {
			glfwDestroyWindow(handle);
		}
		glfwTerminate();
	}
};

Window::Window(int width, int height, const std::string& title) 
	: impl(new Impl(width, height, title)) {}

Window::~Window() {
	delete impl;
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

static Window* window = nullptr;

void InitWindow(int width, int height, const std::string &title) {
	window = new Window(width, height, title);
}

bool WindowShouldClose() {
	return window && window->shouldClose();
}

void Present() {
	if (window) window->swapBuffers();
}

void PollEvents() {
	if (window) window->pollEvents();
}
