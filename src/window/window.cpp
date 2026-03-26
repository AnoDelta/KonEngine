#include "window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "../renderer/opengl/opengl_renderer.hpp"
#include "../time/time.hpp"
#include <functional>

struct Window::Impl {
	struct WindowCallbackData {
		OpenGLRenderer* renderer;
		GLFWwindow* handle;
		Impl* impl;
	};

	GLFWwindow* handle;
	WindowCallbackData callbackData;
	float clearR = 0, clearG = 0, clearB = 0;
	bool vsyncEnabled = true;
	
	Impl(int width, int height, const std::string& title, bool canResize, OpenGLRenderer* renderer) {
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

		callbackData = { renderer, handle, this }; // <-- add this
		glfwSetWindowUserPointer(handle, &callbackData); // <-- replace the old one


		glfwSetWindowRefreshCallback(handle, [](GLFWwindow* win) {
			auto* data = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
			glfwSwapInterval(0);
			// re-run the full frame
			data->renderer->Clear(data->impl->clearR, data->impl->clearG, data->impl->clearB);
			glfwSwapBuffers(data->handle);
			glfwSwapInterval(data->impl->vsyncEnabled ? 1 : 0);
			// glfwPollEvents();
		});

		if (canResize) {
			glfwSetFramebufferSizeCallback(handle, [](GLFWwindow* win, int w, int h) {
				glViewport(0, 0, w, h);
				auto* data = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
				data->renderer->SetProjectionMatrix(w, h);
			});
		}
	}

	~Impl() {
		if (handle) {
			glfwDestroyWindow(handle);
			handle = nullptr;
		}
	}
};

Window::Window(int width, int height, const std::string& title, bool canResize) 
	: impl(nullptr), renderer(std::make_unique<OpenGLRenderer>()) {
		impl = std::make_unique<Impl>(width, height, title, canResize, static_cast<OpenGLRenderer*>(renderer.get()));
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
	impl->clearR = r;
	impl->clearG = g;
	impl->clearB = b;
	renderer->Clear(r, g, b);
}

void Window::setVsync(bool enabled) {
	impl->vsyncEnabled = enabled;
	glfwSwapInterval(enabled ? 1 : 0);
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
void SetVsync(bool enabled) {
	if (window) window->setVsync(enabled);
}

unsigned int Window::loadTexture(const char* path) { return renderer->LoadTexture(path); }
void Window::unloadTexture(unsigned int id) { renderer->UnloadTexture(id); }
void Window::drawTexture(unsigned int id, float x, float y, float w, float h) {
    renderer->DrawTexture(id, x, y, w, h);
}
void Window::drawTextureRec(unsigned int id, float x, float y, float w, float h,
                             float srcX, float srcY, float srcW, float srcH) {
    renderer->DrawTextureRec(id, x, y, w, h, srcX, srcY, srcW, srcH);
}

unsigned int LoadTexture(const char* path) { return window ? window->loadTexture(path) : 0; }
void UnloadTexture(unsigned int id) { if (window) window->unloadTexture(id); }
void DrawTexture(unsigned int id, float x, float y, float w, float h) {
    if (window) window->drawTexture(id, x, y, w, h);
}
void DrawTextureRec(unsigned int id, float x, float y, float w, float h,
                    float srcX, float srcY, float srcW, float srcH) {
    if (window) window->drawTextureRec(id, x, y, w, h, srcX, srcY, srcW, srcH);
}
