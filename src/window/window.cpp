#include "window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "../renderer/opengl/opengl_renderer.hpp"
#include "../time/time.hpp"
#include <functional>
#include "../input/input.hpp"
#include "../camera/camera.hpp"

// -----------------------------------------------------------------------
// Debug mode state — declared before Present() uses it
// -----------------------------------------------------------------------
static bool s_debugMode = false;
void DebugMode(bool enabled) { s_debugMode = enabled; }
bool IsDebugMode()           { return s_debugMode; }

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
                // Force X11 backend — Wayland support in GLFW 3.5 is still unreliable
                // on many compositors. X11 runs via XWayland on Wayland desktops anyway.
#ifdef GLFW_PLATFORM_X11
                glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
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
                glfwShowWindow(handle);
                glfwSwapInterval(1);

                callbackData = { renderer, handle, this };
                glfwSetWindowUserPointer(handle, &callbackData);

                glfwSetWindowRefreshCallback(handle, [](GLFWwindow* win) {
                        auto* data = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
                        glfwSwapInterval(0);
                        data->renderer->Clear(data->impl->clearR, data->impl->clearG, data->impl->clearB);
                        glfwSwapBuffers(data->handle);
                        glfwSwapInterval(data->impl->vsyncEnabled ? 1 : 0);
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
                InitInput(impl->handle);
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
        if (!window) return;

        if (s_debugMode) {
            // FPS counter
            static float s_dbgTimer  = 0.0f;
            static int   s_dbgFPS    = 0;
            static int   s_dbgFrames = 0;
            s_dbgFrames++;
            s_dbgTimer += GetDeltaTime();
            if (s_dbgTimer >= 1.0f) {
                s_dbgFPS    = s_dbgFrames;
                s_dbgFrames = 0;
                s_dbgTimer  = 0.0f;
                std::cout << "[KonEngine DEBUG]"
                          << "  FPS: "    << s_dbgFPS
                          << "  Mouse: (" << (int)GetMouseX()
                          << ", "         << (int)GetMouseY() << ")"
                          << "  dt: "     << GetDeltaTime()
                          << "\n";
            }

            // Red border
            int W = window->getWidth();
            int H = window->getHeight();
            float t = 2.0f;
            window->drawRectangle(0,       0,       (float)W, t,        1,0,0,1);
            window->drawRectangle(0,       (float)H-t, (float)W, t,     1,0,0,1);
            window->drawRectangle(0,       0,       t, (float)H,        1,0,0,1);
            window->drawRectangle((float)W-t, 0,   t, (float)H,        1,0,0,1);

            // Mouse crosshair
            float mx = GetMouseX(), my = GetMouseY();
            float cs = 8.0f;
            window->drawLine(mx-cs, my,    mx+cs, my,    1,0,0,1);
            window->drawLine(mx,    my-cs, mx,    my+cs, 1,0,0,1);


			// Log mouse clicks
			if (IsMouseButtonPressed(Mouse::Left))
				std::cout << "[KonEngine DEBUG] Mouse: LEFT clicked at ("
					<< (int)mx << ", " << (int)my << ")\n";
			if (IsMouseButtonPressed(Mouse::Right))
				std::cout << "[KonEngine DEBUG] Mouse: RIGHT clicked at ("
					<< (int)mx << ", " << (int)my << ")\n";
			if (IsMouseButtonPressed(Mouse::Middle))
				std::cout << "[KonEngine DEBUG] Mouse: MIDDLE clicked at ("
					<< (int)mx << ", " << (int)my << ")\n";
        }

        window->swapBuffers();
}

void PollEvents() {
        if (window) {
                UpdateInput();
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

Texture Window::loadTexture(const char* path) { return renderer->LoadTexture(path); }
void Window::unloadTexture(Texture& texture) { renderer->UnloadTexture(texture); }
void Window::drawTexture(Texture& texture, float x, float y, float w, float h) {
    renderer->DrawTexture(texture, x, y, w, h);
}
void Window::drawTextureRec(Texture& texture, float x, float y, float w, float h,
                             float srcX, float srcY, float srcW, float srcH) {
    renderer->DrawTextureRec(texture, x, y, w, h, srcX, srcY, srcW, srcH);
}
void Window::drawTexture(Texture& texture, float x, float y, float w, float h, Color tint) {
    renderer->DrawTexture(texture, x, y, w, h, tint);
}
void Window::drawTextureRec(Texture& texture, float x, float y, float w, float h,
                             float srcX, float srcY, float srcW, float srcH, Color tint) {
    renderer->DrawTextureRec(texture, x, y, w, h, srcX, srcY, srcW, srcH, tint);
}

Texture LoadTexture(const char* path) { return window ? window->loadTexture(path) : Texture{0,0,0}; }
void UnloadTexture(Texture& texture) { if (window) window->unloadTexture(texture); }
void DrawTexture(Texture& texture, float x, float y, float w, float h) {
    if (window) window->drawTexture(texture, x, y, w, h);
}
void DrawTextureRec(Texture& texture, float x, float y, float w, float h,
                    float srcX, float srcY, float srcW, float srcH) {
    if (window) window->drawTextureRec(texture, x, y, w, h, srcX, srcY, srcW, srcH);
}
void DrawTexture(Texture& texture, float x, float y, float w, float h, Color tint) {
    if (window) window->drawTexture(texture, x, y, w, h, tint);
}
void DrawTextureRec(Texture& texture, float x, float y, float w, float h,
                    float srcX, float srcY, float srcW, float srcH, Color tint) {
    if (window) window->drawTextureRec(texture, x, y, w, h, srcX, srcY, srcW, srcH, tint);
}

void Window::drawRectangle(float x, float y, float w, float h, Color color) {
    renderer->DrawRectangle(x, y, w, h, color);
}
void Window::drawCircle(float x, float y, float radius, Color color) {
    renderer->DrawCircle(x, y, radius, color);
}
void Window::drawLine(float x1, float y1, float x2, float y2, Color color) {
    renderer->DrawLine(x1, y1, x2, y2, color);
}

void DrawRectangle(float x, float y, float w, float h, Color color) {
    if (window) window->drawRectangle(x, y, w, h, color);
}
void DrawCircle(float x, float y, float radius, Color color) {
    if (window) window->drawCircle(x, y, radius, color);
}
void DrawLine(float x1, float y1, float x2, float y2, Color color) {
    if (window) window->drawLine(x1, y1, x2, y2, color);
}

void Window::drawGlyph(unsigned int atlasID, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, Color color) {
    renderer->DrawGlyph(atlasID, x, y, w, h, u0, v0, u1, v1, color);
}

void DrawGlyph(unsigned int atlasID, float x, float y, float w, float h,
               float u0, float v0, float u1, float v1, Color color) {
    if (window) window->drawGlyph(atlasID, x, y, w, h, u0, v0, u1, v1, color);
}

void Window::beginCamera2D(const Camera2D& cam) {
    renderer->BeginCamera2D(cam);
}
void Window::endCamera2D() {
    renderer->EndCamera2D();
}

void BeginCamera2D(const Camera2D& cam) {
    if (window) window->beginCamera2D(cam);
}
void EndCamera2D() {
    if (window) window->endCamera2D();
}
