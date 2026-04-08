#include "window.hpp"
#include "../asset_manager.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <memory>
#include "../renderer/opengl/opengl_renderer.hpp"
#include "../time/time.hpp"
#include <functional>
#include <cstdio>
#include "../stb/stb_image.h"
#include "../input/input.hpp"
#include "../camera/camera.hpp"
#include "../font/font.hpp"
#include <cmath>
#include <chrono>
#include <algorithm>

static bool s_debugMode = false;
static Camera2D s_lastCamera = {};
static bool s_hasCameraThisFrame = false;
void DebugMode(bool enabled) { s_debugMode = enabled; }
bool IsDebugMode()           { return s_debugMode; }

// ── Letterbox scaling state ──────────────────────────────────────────────
static int   s_designW = 0, s_designH = 0;   // design (logical) resolution
static float s_lbScale   = 1.0f;              // fit scale factor
static float s_lbOffsetX = 0.0f;              // viewport X offset (black bar)
static float s_lbOffsetY = 0.0f;              // viewport Y offset (black bar)
static int   s_lbViewW   = 0, s_lbViewH = 0;  // scaled viewport size in pixels
static bool  s_letterboxEnabled = false;       // true when window is resizable

static void RecalcLetterbox(int actualW, int actualH) {
    if (s_designW <= 0 || s_designH <= 0) return;
    float scaleX = (float)actualW / (float)s_designW;
    float scaleY = (float)actualH / (float)s_designH;
    s_lbScale  = (scaleX < scaleY) ? scaleX : scaleY;
    s_lbViewW  = (int)(s_designW * s_lbScale);
    s_lbViewH  = (int)(s_designH * s_lbScale);
    s_lbOffsetX = (actualW - s_lbViewW) / 2.0f;
    s_lbOffsetY = (actualH - s_lbViewH) / 2.0f;
}

struct Window::Impl {
	struct WindowCallbackData {
		OpenGLRenderer* renderer;
		GLFWwindow* handle;
		Impl* impl;
	};
	GLFWwindow* handle;
	WindowCallbackData callbackData;
	float clearR = 0, clearG = 0, clearB = 0;
	bool vsyncEnabled = false;

	Impl(int width, int height, const std::string& title, bool canResize, OpenGLRenderer* renderer) {
#if defined(GLFW_PLATFORM_X11) && !defined(_WIN32)
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
		if (!glfwInit()) { std::cerr << "Failed to initialize GLFW\n"; handle = nullptr; return; }
		if (!canResize) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		else            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!handle) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); return; }
		glfwMakeContextCurrent(handle);
		glfwShowWindow(handle);
		glfwSwapInterval(0);
		callbackData = { renderer, handle, this };
		glfwSetWindowUserPointer(handle, &callbackData);
		glfwSetWindowRefreshCallback(handle, [](GLFWwindow* win) {
			auto* d = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
			glfwSwapInterval(0);
			d->renderer->Clear(d->impl->clearR, d->impl->clearG, d->impl->clearB);
			glfwSwapBuffers(d->handle);
			glfwSwapInterval(d->impl->vsyncEnabled ? 1 : 0);
		});
		if (canResize) {
			glfwSetFramebufferSizeCallback(handle, [](GLFWwindow* win, int w, int h) {
				auto* d = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
				if (s_letterboxEnabled) {
					RecalcLetterbox(w, h);
					// Projection stays at design resolution — the viewport does the scaling
					glViewport((int)s_lbOffsetX, (int)s_lbOffsetY, s_lbViewW, s_lbViewH);
					d->renderer->SetProjectionMatrix(s_designW, s_designH);
				} else {
					glViewport(0, 0, w, h);
					d->renderer->SetProjectionMatrix(w, h);
				}
			});
		}
	}
	~Impl() { if (handle) { glfwDestroyWindow(handle); handle = nullptr; } }
};

Window::Window(int width, int height, const std::string& title, bool canResize)
	: impl(nullptr), renderer(std::make_unique<OpenGLRenderer>()) {
	impl = std::make_unique<Impl>(width, height, title, canResize,
	                              static_cast<OpenGLRenderer*>(renderer.get()));
	renderer->Init();
	InitInput(impl->handle);
	static_cast<OpenGLRenderer*>(renderer.get())->SetProjectionMatrix(width, height);
}
Window::~Window() { renderer.reset(); impl.reset(); }

void Window::pollEvents() { glfwPollEvents(); }
bool Window::shouldClose() const { return !impl->handle || glfwWindowShouldClose(impl->handle); }
void Window::swapBuffers() { if (impl->handle) glfwSwapBuffers(impl->handle); }
int  Window::getWidth()  { int w,h; glfwGetFramebufferSize(impl->handle,&w,&h); return w; }
int  Window::getHeight() { int w,h; glfwGetFramebufferSize(impl->handle,&w,&h); return h; }
void Window::clearBackground(float r, float g, float b) {
	impl->clearR=r; impl->clearG=g; impl->clearB=b; renderer->Clear(r,g,b);
}
void Window::clearBackground(Color color) {
	clearBackground(color.r, color.g, color.b);
}
void Window::setVsync(bool e) { impl->vsyncEnabled=e; glfwSwapInterval(e?1:0); }

void Window::setIcon(const char* path) {
    int w, h, channels;
    unsigned char* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) return;

    GLFWimage icon;
    icon.width  = w;
    icon.height = h;
    icon.pixels = pixels;
    glfwSetWindowIcon(impl->handle, 1, &icon);
    stbi_image_free(pixels);
}

static Window* window = nullptr;

// ── Engine splash screen ─────────────────────────────────────────────────
static void RunSplashScreen(int w, int h) {
    if (!window) return;

    // Try to load logo.png for splash
    bool hasLogo = false;
    Texture logo = {0, 0, 0};
    {
        FILE* f = fopen("logo.png", "rb");
        if (f) { fclose(f); logo = window->loadTexture("logo.png"); hasLogo = (logo.id != 0); }
    }

    // Splash timing: 0.3s fade in, 0.8s hold, 0.4s fade out = 1.5s total
    const float fadeIn = 0.3f, hold = 0.8f, fadeOut = 0.4f;
    const float total = fadeIn + hold + fadeOut;

    auto startTime = std::chrono::high_resolution_clock::now();
    float elapsed = 0.0f;

    while (elapsed < total && !glfwWindowShouldClose(glfwGetCurrentContext())) {
        auto now = std::chrono::high_resolution_clock::now();
        elapsed = std::chrono::duration<float>(now - startTime).count();

        // Calculate alpha: fade in → hold → fade out
        float alpha = 1.0f;
        if (elapsed < fadeIn) {
            alpha = elapsed / fadeIn;
        } else if (elapsed > fadeIn + hold) {
            alpha = 1.0f - (elapsed - fadeIn - hold) / fadeOut;
        }
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        // Clear to black
        window->clearBackground(0.0f, 0.0f, 0.0f);

        float cx = w * 0.5f, cy = h * 0.5f;

        if (hasLogo) {
            // Draw logo centered with alpha
            float lw = (float)logo.width, lh = (float)logo.height;
            // Scale down if larger than 60% of window
            float maxW = w * 0.6f, maxH = h * 0.6f;
            if (lw > maxW || lh > maxH) {
                float s = std::min(maxW / lw, maxH / lh);
                lw *= s; lh *= s;
            }
            window->drawTexture(logo, cx - lw*0.5f, cy - lh*0.5f, lw, lh,
                                Color{1.0f, 1.0f, 1.0f, alpha});
        } else {
            // No logo — draw a triangle + engine name
            // Triangle (pointing up)
            float ts = 40.0f; // triangle size
            float ty = cy - 30.0f;
            // Draw triangle as 3 thin rectangles approximating a filled triangle
            for (float row = 0; row < ts; row += 2.0f) {
                float frac = row / ts;
                float hw = ts * 0.5f * frac;
                window->drawRectangle(cx - hw, ty + row, hw * 2.0f, 2.0f,
                                      0.3f * alpha, 0.7f * alpha, 1.0f * alpha, alpha);
            }
        }

        // "KonEngine" text below logo/triangle
        extern void DrawText(const char*, float, float, Color);
        Color textCol = {0.6f * alpha, 0.6f * alpha, 0.7f * alpha, alpha};
        DrawText("KonEngine", cx - 50.0f, cy + 50.0f, textCol);

        window->present();
        window->swapBuffers();
        glfwPollEvents();
    }

    // Clean up logo texture
    if (hasLogo) window->unloadTexture(logo);
}

void InitWindow(int w, int h, const std::string& t, bool r) {
    s_designW = w;
    s_designH = h;
    s_letterboxEnabled = r;
    if (r) {
        RecalcLetterbox(w, h);
    }
    window = new Window(w,h,t,r);

    // Auto-load window icon from logo.png if it exists
    FILE* f = fopen("logo.png", "rb");
    if (f) { fclose(f); SetWindowIcon("logo.png"); }

    // Show engine splash screen
    RunSplashScreen(w, h);
}
bool WindowShouldClose() { return window && window->shouldClose(); }

void Present() {
	if (!window) return;

	// Flush any batched rectangles before debug overlay or swap
	window->present();

	if (s_debugMode) {
		static float dbgTimer=0; static int dbgFPS=0, dbgFrames=0;
		static float dbgDt=0;
		dbgFrames++;
		dbgDt    = GetDeltaTime();
		dbgTimer += dbgDt;
		if (dbgTimer >= 1.0f) {
			dbgFPS=dbgFrames; dbgFrames=0; dbgTimer=0;
		}

		int W = (s_letterboxEnabled && s_designW > 0) ? s_designW : window->getWidth();
		int H = (s_letterboxEnabled && s_designH > 0) ? s_designH : window->getHeight();

		// ── World-space debug grid (uses thin rectangles, not GL_LINES) ──
		{
			Camera2D gridCam = s_hasCameraThisFrame
				? s_lastCamera
				: Camera2D{(float)W * 0.5f, (float)H * 0.5f, 1.0f, 0.0f};
			window->beginCamera2D(gridCam);

			float zoom  = gridCam.zoom;
			float camX  = gridCam.x, camY = gridCam.y;
			float halfW = (W*0.5f)/zoom,  halfH = (H*0.5f)/zoom;
			float left  = camX-halfW, right  = camX+halfW;
			float top   = camY-halfH, bottom = camY+halfH;

			// Fine grid (32px) — 1px wide rectangles
			float step = 32.0f;
			if (step * zoom >= 4.0f) {
				float sx = floorf(left/step)*step;
				float sy = floorf(top/step)*step;
				for (float x=sx; x<=right; x+=step)
					window->drawRectangle(x, top, 1.0f/zoom, bottom-top, 0.4f,0.4f,0.5f,0.2f);
				for (float y=sy; y<=bottom; y+=step)
					window->drawRectangle(left, y, right-left, 1.0f/zoom, 0.4f,0.4f,0.5f,0.2f);
			}

			// Coarse grid (256px)
			step = 256.0f;
			if (step * zoom >= 4.0f) {
				float sx = floorf(left/step)*step;
				float sy = floorf(top/step)*step;
				for (float x=sx; x<=right; x+=step)
					window->drawRectangle(x, top, 1.0f/zoom, bottom-top, 0.5f,0.5f,0.7f,0.3f);
				for (float y=sy; y<=bottom; y+=step)
					window->drawRectangle(left, y, right-left, 1.0f/zoom, 0.5f,0.5f,0.7f,0.3f);
			}

			// World origin axes (2px wide)
			window->drawRectangle(left, -1.0f/zoom, right-left, 2.0f/zoom, 0.6f,0.6f,0.9f,0.4f);
			window->drawRectangle(-1.0f/zoom, top, 2.0f/zoom, bottom-top, 0.6f,0.6f,0.9f,0.4f);

			window->endCamera2D();
		}

		// ── Screen-space HUD ──

		// Stats bar
		char buf[256];
		snprintf(buf, sizeof(buf),
			"FPS: %d  dt: %.4f  Mouse: (%d, %d)  Zoom: %.2f",
			dbgFPS, dbgDt,
			(int)GetMouseX(), (int)GetMouseY(),
			s_hasCameraThisFrame ? s_lastCamera.zoom : 1.0f);
		window->drawRectangle(0, 0, (float)W, 18, 0,0,0,0.65f);
		extern void DrawText(const char*, float, float, Color);
		Color cyan = {0.3f, 1.0f, 0.9f, 1.0f};
		DrawText(buf, 4, 2, cyan);

		// Red border
		float t=2.0f;
		window->drawRectangle(0,       0,       (float)W, t,      1,0,0,1);
		window->drawRectangle(0,       (float)H-t, (float)W, t,   1,0,0,1);
		window->drawRectangle(0,       0,       t, (float)H,      1,0,0,1);
		window->drawRectangle((float)W-t, 0,   t, (float)H,      1,0,0,1);

		// Mouse crosshair (thin rectangles instead of GL_LINES)
		float mx = s_letterboxEnabled ? GetGameMouseX() : GetMouseX();
		float my = s_letterboxEnabled ? GetGameMouseY() : GetMouseY();
		float cs=8.0f;
		window->drawRectangle(mx-cs, my-0.5f, cs*2, 1.0f, 1,0,0,1);
		window->drawRectangle(mx-0.5f, my-cs, 1.0f, cs*2, 1,0,0,1);

		s_hasCameraThisFrame = false;

		if (IsMouseButtonPressed(Mouse::Left))
			std::cout << "[KonEngine DEBUG] Mouse: LEFT clicked at (" << (int)mx << ", " << (int)my << ")" << std::endl;
		if (IsMouseButtonPressed(Mouse::Right))
			std::cout << "[KonEngine DEBUG] Mouse: RIGHT clicked at (" << (int)mx << ", " << (int)my << ")" << std::endl;
		if (IsMouseButtonPressed(Mouse::Middle))
			std::cout << "[KonEngine DEBUG] Mouse: MIDDLE clicked at (" << (int)mx << ", " << (int)my << ")" << std::endl;
		float sc = GetMouseScroll();
		if (sc != 0.0f) std::cout << "[KonEngine DEBUG] Mouse: SCROLL " << sc << std::endl;
	}

	// Final flush: push any remaining batched geometry (debug overlay, etc.)
	window->present();

	// Draw letterbox black bars LAST, on top of everything using scissor test
	if (s_letterboxEnabled) {
		int fbW = window->getWidth(), fbH = window->getHeight();
		glViewport(0, 0, fbW, fbH);
		glEnable(GL_SCISSOR_TEST);
		// Left bar
		if (s_lbOffsetX > 0) {
			glScissor(0, 0, (int)s_lbOffsetX, fbH);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		// Right bar
		int rightX = (int)(s_lbOffsetX + s_lbViewW);
		if (rightX < fbW) {
			glScissor(rightX, 0, fbW - rightX, fbH);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		// Top bar (OpenGL Y is bottom-up, so "top" in screen = high Y in GL)
		int topY = (int)(s_lbOffsetY + s_lbViewH);
		if (topY < fbH) {
			glScissor(0, topY, fbW, fbH - topY);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		// Bottom bar
		if (s_lbOffsetY > 0) {
			glScissor(0, 0, fbW, (int)s_lbOffsetY);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glDisable(GL_SCISSOR_TEST);
		// Restore letterbox viewport for next frame
		glViewport((int)s_lbOffsetX, (int)s_lbOffsetY, s_lbViewW, s_lbViewH);
	}

	window->swapBuffers();
}

void PollEvents() { if (window) { UpdateInput(); TickTime(); window->pollEvents(); } }
void ClearBackground(float r, float g, float b) {
    if (!window) return;
    if (s_letterboxEnabled) {
        int fbW = window->getWidth(), fbH = window->getHeight();
        // 1. Clear entire framebuffer with black (letterbox bars)
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 2. Set viewport to the letterboxed region and clear with game color
        RecalcLetterbox(fbW, fbH);
        glViewport((int)s_lbOffsetX, (int)s_lbOffsetY, s_lbViewW, s_lbViewH);
        window->clearBackground(r, g, b);
    } else {
        window->clearBackground(r, g, b);
    }
}
void ClearBackground(Color color) {
    ClearBackground(color.r, color.g, color.b);
}
int  GetWindowWidth()  {
    if (s_letterboxEnabled && s_designW > 0) return s_designW;
    return window ? window->getWidth()  : 0;
}
int  GetWindowHeight() {
    if (s_letterboxEnabled && s_designH > 0) return s_designH;
    return window ? window->getHeight() : 0;
}

void Window::drawRectangle(float x,float y,float w,float h,float r,float g,float b,float a){renderer->DrawRectangle(x,y,w,h,r,g,b,a);}
void Window::drawCircle(float x,float y,float r2,float r,float g,float b,float a){renderer->DrawCircle(x,y,r2,r,g,b,a);}
void Window::drawLine(float x1,float y1,float x2,float y2,float r,float g,float b,float a){renderer->DrawLine(x1,y1,x2,y2,r,g,b,a);}
void DrawRectangle(float x,float y,float w,float h,float r,float g,float b,float a){if(window)window->drawRectangle(x,y,w,h,r,g,b,a);}
void DrawCircle(float x,float y,float r2,float r,float g,float b,float a){if(window)window->drawCircle(x,y,r2,r,g,b,a);}
void DrawLine(float x1,float y1,float x2,float y2,float r,float g,float b,float a){if(window)window->drawLine(x1,y1,x2,y2,r,g,b,a);}
void SetVsync(bool e){if(window)window->setVsync(e);}

void SetWindowIcon(const char* path) { if(window)window->setIcon(path); }

static bool s_fullscreen = false;
static int s_windowedX = 0, s_windowedY = 0;
static int s_windowedW = 0, s_windowedH = 0;

void SetFullscreen(bool enabled) {
    if (!window) return;
    GLFWwindow* handle = glfwGetCurrentContext();
    if (!handle) return;
    if (enabled && !s_fullscreen) {
        // Save windowed position/size
        glfwGetWindowPos(handle, &s_windowedX, &s_windowedY);
        glfwGetWindowSize(handle, &s_windowedW, &s_windowedH);
        // Go fullscreen on primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        s_fullscreen = true;
    } else if (!enabled && s_fullscreen) {
        // Restore windowed mode
        glfwSetWindowMonitor(handle, nullptr, s_windowedX, s_windowedY,
                             s_windowedW, s_windowedH, 0);
        s_fullscreen = false;
    }
}
bool IsFullscreen() { return s_fullscreen; }

Texture Window::loadTexture(const char* p){return renderer->LoadTexture(p);}
void    Window::unloadTexture(Texture& t){renderer->UnloadTexture(t);}
void Window::drawTexture(Texture& t,float x,float y,float w,float h){renderer->DrawTexture(t,x,y,w,h);}
void Window::drawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh){renderer->DrawTextureRec(t,x,y,w,h,sx,sy,sw,sh);}
void Window::drawTexture(Texture& t,float x,float y,float w,float h,Color c){renderer->DrawTexture(t,x,y,w,h,c);}
void Window::drawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh,Color c){renderer->DrawTextureRec(t,x,y,w,h,sx,sy,sw,sh,c);}
Texture LoadTexture(const char* p){
    if (!window) {
        std::cerr << "[KonEngine] warning: LoadTexture(\"" << p << "\") called before InitWindow — texture will be blank\n";
        return Texture{0,0,0};
    }
    std::string r=AssetManager::resolvePath(p);
    return window->loadTexture(r.c_str());
}
void    UnloadTexture(Texture& t){if(window)window->unloadTexture(t);}
void DrawTexture(Texture& t,float x,float y,float w,float h){if(window)window->drawTexture(t,x,y,w,h);}
void DrawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh){if(window)window->drawTextureRec(t,x,y,w,h,sx,sy,sw,sh);}
void DrawTexture(Texture& t,float x,float y,float w,float h,Color c){if(window)window->drawTexture(t,x,y,w,h,c);}
void DrawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh,Color c){if(window)window->drawTextureRec(t,x,y,w,h,sx,sy,sw,sh,c);}

void Window::drawRectangle(float x,float y,float w,float h,Color c){renderer->DrawRectangle(x,y,w,h,c);}
void Window::drawCircle(float x,float y,float r,Color c){renderer->DrawCircle(x,y,r,c);}
void Window::drawLine(float x1,float y1,float x2,float y2,Color c){renderer->DrawLine(x1,y1,x2,y2,c);}
void DrawRectangle(float x,float y,float w,float h,Color c){if(window)window->drawRectangle(x,y,w,h,c);}
void DrawCircle(float x,float y,float r,Color c){if(window)window->drawCircle(x,y,r,c);}
void DrawLine(float x1,float y1,float x2,float y2,Color c){if(window)window->drawLine(x1,y1,x2,y2,c);}

void Window::drawGlyph(unsigned int id,float x,float y,float w,float h,float u0,float v0,float u1,float v1,Color c){renderer->DrawGlyph(id,x,y,w,h,u0,v0,u1,v1,c);}
void DrawGlyph(unsigned int id,float x,float y,float w,float h,float u0,float v0,float u1,float v1,Color c){if(window)window->drawGlyph(id,x,y,w,h,u0,v0,u1,v1,c);}

void Window::beginCamera2D(const Camera2D& cam){renderer->BeginCamera2D(cam);}
void Window::endCamera2D(){renderer->EndCamera2D();}
void Window::present(){renderer->Present();}

void BeginCamera2D(const Camera2D& cam) {
	if (!window) return;
	// Clamp zoom — extreme values cause world rotation/freeze artifacts
	Camera2D safe = cam;
	if (safe.zoom < 0.05f) safe.zoom = 0.05f;
	if (safe.zoom > 50.0f) safe.zoom = 50.0f;
	s_lastCamera         = safe;
	s_hasCameraThisFrame = true;
	window->beginCamera2D(safe);
}
void EndCamera2D() { if (window) window->endCamera2D(); }

// ── Letterbox getters ─────────────────────────────────────────────────────
int   GetDesignWidth()      { return s_designW; }
int   GetDesignHeight()     { return s_designH; }
float GetLetterboxScale()   { return s_lbScale; }
float GetLetterboxOffsetX() { return s_lbOffsetX; }
float GetLetterboxOffsetY() { return s_lbOffsetY; }

// Helper: get window (not framebuffer) size for cursor coordinate scaling
static void GetWindowSizePixels(int& w, int& h) {
    if (window) {
        w = window->getWidth();
        h = window->getHeight();
    } else { w = s_designW; h = s_designH; }
}

float GetGameMouseX() {
    // Convert raw cursor position to design-resolution coordinates
    if (s_letterboxEnabled && s_lbScale > 0.0f) {
        return (GetMouseX() - s_lbOffsetX) / s_lbScale;
    }
    // No letterbox: scale from actual framebuffer size to design size
    int fbW, fbH;
    GetWindowSizePixels(fbW, fbH);
    if (s_designW > 0 && fbW > 0) {
        return GetMouseX() * (float)s_designW / (float)fbW;
    }
    return GetMouseX();
}
float GetGameMouseY() {
    if (s_letterboxEnabled && s_lbScale > 0.0f) {
        return (GetMouseY() - s_lbOffsetY) / s_lbScale;
    }
    int fbW, fbH;
    GetWindowSizePixels(fbW, fbH);
    if (s_designH > 0 && fbH > 0) {
        return GetMouseY() * (float)s_designH / (float)fbH;
    }
    return GetMouseY();
}

float GetWorldMouseX(const Camera2D& cam) {
    // Always use design resolution for world-space conversion
    float mx = GetGameMouseX();
    float dw = (s_designW > 0) ? (float)s_designW : (float)GetWindowWidth();
    return cam.x + (mx - dw * 0.5f) / cam.zoom;
}
float GetWorldMouseY(const Camera2D& cam) {
    float my = GetGameMouseY();
    float dh = (s_designH > 0) ? (float)s_designH : (float)GetWindowHeight();
    return cam.y + (my - dh * 0.5f) / cam.zoom;
}
