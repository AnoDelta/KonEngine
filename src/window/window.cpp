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
#include <cmath>

static bool s_debugMode = false;
static Camera2D s_lastCamera = {};
static bool s_hasCameraThisFrame = false;
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
		glfwSwapInterval(1);
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
				glViewport(0, 0, w, h);
				auto* d = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(win));
				d->renderer->SetProjectionMatrix(w, h);
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
void Window::setVsync(bool e) { impl->vsyncEnabled=e; glfwSwapInterval(e?1:0); }

static Window* window = nullptr;
void InitWindow(int w, int h, const std::string& t, bool r) { window = new Window(w,h,t,r); }
bool WindowShouldClose() { return window && window->shouldClose(); }

void Present() {
	if (!window) return;

	if (s_debugMode) {
		static float dbgTimer=0; static int dbgFPS=0, dbgFrames=0;
		dbgFrames++;
		dbgTimer += GetDeltaTime();
		if (dbgTimer >= 1.0f) {
			dbgFPS=dbgFrames; dbgFrames=0; dbgTimer=0;
			std::cout << "[KonEngine DEBUG]  FPS: " << dbgFPS
			          << "  Mouse: (" << (int)GetMouseX() << ", " << (int)GetMouseY() << ")"
			          << "  dt: " << GetDeltaTime() << "\n";
		}

		int W=window->getWidth(), H=window->getHeight();
		float t=2.0f;
		window->drawRectangle(0,       0,       (float)W, t,      1,0,0,1);
		window->drawRectangle(0,       (float)H-t, (float)W, t,   1,0,0,1);
		window->drawRectangle(0,       0,       t, (float)H,      1,0,0,1);
		window->drawRectangle((float)W-t, 0,   t, (float)H,      1,0,0,1);

		float mx=GetMouseX(), my=GetMouseY(), cs=8.0f;
		window->drawLine(mx-cs, my,    mx+cs, my,    1,0,0,1);
		window->drawLine(mx,    my-cs, mx,    my+cs, 1,0,0,1);

		// Debug grid -- strict pixel density check to prevent lag
		if (s_hasCameraThisFrame) {
			window->beginCamera2D(s_lastCamera);

			float zoom  = s_lastCamera.zoom;
			float camX  = s_lastCamera.x, camY = s_lastCamera.y;
			float halfW = (W*0.5f)/zoom,  halfH = (H*0.5f)/zoom;
			float left  = camX-halfW, right  = camX+halfW;
			float top   = camY-halfH, bottom = camY+halfH;

			// Only draw a grid tier if each line would be at least 4 pixels apart on screen.
			// gridStep * zoom >= 4  =>  gridStep >= 4/zoom
			// This naturally hides fine grid when zoomed out without needing a line count.
			float minPixelGap = 4.0f;

			// Fine grid (32px world units)
			if (32.0f * zoom >= minPixelGap) {
				float s = 32.0f;
				float sx = floorf(left/s)*s, sy = floorf(top/s)*s;
				for (float x=sx; x<=right;  x+=s) window->drawLine(x,top,x,bottom, 0.22f,0.22f,0.30f,1);
				for (float y=sy; y<=bottom; y+=s) window->drawLine(left,y,right,y,  0.22f,0.22f,0.30f,1);
			}

			// Coarse grid (256px world units)
			if (256.0f * zoom >= minPixelGap) {
				float s = 256.0f;
				float sx = floorf(left/s)*s, sy = floorf(top/s)*s;
				for (float x=sx; x<=right;  x+=s) window->drawLine(x,top,x,bottom, 0.38f,0.38f,0.50f,1);
				for (float y=sy; y<=bottom; y+=s) window->drawLine(left,y,right,y,  0.38f,0.38f,0.50f,1);
			}

			// World origin axes -- always 2 lines, no condition
			window->drawLine(left,0, right,0,  0.55f,0.55f,0.75f,1);
			window->drawLine(0,top,  0,bottom,  0.55f,0.55f,0.75f,1);

			window->endCamera2D();
		}
		s_hasCameraThisFrame = false;

		if (IsMouseButtonPressed(Mouse::Left))
			std::cout << "[KonEngine DEBUG] Mouse: LEFT clicked at (" << (int)mx << ", " << (int)my << ")\n";
		if (IsMouseButtonPressed(Mouse::Right))
			std::cout << "[KonEngine DEBUG] Mouse: RIGHT clicked at (" << (int)mx << ", " << (int)my << ")\n";
		if (IsMouseButtonPressed(Mouse::Middle))
			std::cout << "[KonEngine DEBUG] Mouse: MIDDLE clicked at (" << (int)mx << ", " << (int)my << ")\n";
		float sc = GetMouseScroll();
		if (sc != 0.0f) std::cout << "[KonEngine DEBUG] Mouse: SCROLL " << sc << "\n";
	}

	window->swapBuffers();
}

void PollEvents() { if (window) { UpdateInput(); TickTime(); window->pollEvents(); } }
void ClearBackground(float r, float g, float b) { if (window) window->clearBackground(r,g,b); }
int  GetWindowWidth()  { return window ? window->getWidth()  : 0; }
int  GetWindowHeight() { return window ? window->getHeight() : 0; }

void Window::drawRectangle(float x,float y,float w,float h,float r,float g,float b,float a){renderer->DrawRectangle(x,y,w,h,r,g,b,a);}
void Window::drawCircle(float x,float y,float r2,float r,float g,float b,float a){renderer->DrawCircle(x,y,r2,r,g,b,a);}
void Window::drawLine(float x1,float y1,float x2,float y2,float r,float g,float b,float a){renderer->DrawLine(x1,y1,x2,y2,r,g,b,a);}
void DrawRectangle(float x,float y,float w,float h,float r,float g,float b,float a){if(window)window->drawRectangle(x,y,w,h,r,g,b,a);}
void DrawCircle(float x,float y,float r2,float r,float g,float b,float a){if(window)window->drawCircle(x,y,r2,r,g,b,a);}
void DrawLine(float x1,float y1,float x2,float y2,float r,float g,float b,float a){if(window)window->drawLine(x1,y1,x2,y2,r,g,b,a);}
void SetVsync(bool e){if(window)window->setVsync(e);}

Texture Window::loadTexture(const char* p){return renderer->LoadTexture(p);}
void    Window::unloadTexture(Texture& t){renderer->UnloadTexture(t);}
void Window::drawTexture(Texture& t,float x,float y,float w,float h){renderer->DrawTexture(t,x,y,w,h);}
void Window::drawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh){renderer->DrawTextureRec(t,x,y,w,h,sx,sy,sw,sh);}
void Window::drawTexture(Texture& t,float x,float y,float w,float h,Color c){renderer->DrawTexture(t,x,y,w,h,c);}
void Window::drawTextureRec(Texture& t,float x,float y,float w,float h,float sx,float sy,float sw,float sh,Color c){renderer->DrawTextureRec(t,x,y,w,h,sx,sy,sw,sh,c);}
Texture LoadTexture(const char* p){return window?window->loadTexture(p):Texture{0,0,0};}
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

void BeginCamera2D(const Camera2D& cam) {
	s_lastCamera = cam;
	s_hasCameraThisFrame = true;
	if (window) window->beginCamera2D(cam);
}
void EndCamera2D() { if (window) window->endCamera2D(); }

float GetWorldMouseX(const Camera2D& cam) { return cam.x+(GetMouseX()-GetWindowWidth() *0.5f)/cam.zoom; }
float GetWorldMouseY(const Camera2D& cam) { return cam.y+(GetMouseY()-GetWindowHeight()*0.5f)/cam.zoom; }
