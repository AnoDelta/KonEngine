#pragma once

#include <string>
#include <memory>
#include "../renderer/renderer.hpp"
#include "../color/color.hpp"
#include "../renderer/texture.hpp"

class Window {
public:
	Window(int width, int height, const std::string& title, bool canResize = false);
	~Window();

	void pollEvents();
	bool shouldClose() const;
	void swapBuffers();
	void clearBackground(float r, float g, float b);
	void drawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
    void drawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);       
    void drawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0f);
	void drawRectangle(float x, float y, float w, float h, Color color);
	void drawCircle(float x, float y, float radius, Color color);
	void drawLine(float x1, float y1, float x2, float y2, Color color);
	int getHeight();
	int getWidth();
	void setVsync(bool enabled);

	Texture loadTexture(const char* path);
	void unloadTexture(Texture& texture);
	void drawTexture(Texture& texture, float x, float y, float width, float height);
	void drawTextureRec(Texture& texture, float x, float y, float width, float height,
					 float srcX, float srcY, float srcWidth, float srcHeight);
	void drawTexture(Texture& texture, float x, float y, float width, float height, Color tint);
	void drawTextureRec(Texture& texture, float x, float y, float width, float height,
					 float srcX, float srcY, float srcWidth, float srcHeight, Color tint);

	void drawGlyph(unsigned int atlasID, float x, float y, float w, float h,
				   float u0, float v0, float u1, float v1, Color color);

	void beginCamera2D(const Camera2D& cam);
	void endCamera2D();

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	std::unique_ptr<IRenderer> renderer;
};

void InitWindow(int width, int height, const std::string& title, bool canResize = false);
bool WindowShouldClose();
void Present();
void PollEvents();
void ClearBackground(float r, float g, float b);

void SetVsync(bool enabled);

int GetWindowWidth();
int GetWindowHeight();

void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0f);

Texture LoadTexture(const char* path);
void UnloadTexture(Texture& texture);
void DrawTexture(Texture& texture, float x, float y, float width, float height);
void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
                    float srcX, float srcY, float srcWidth, float srcHeight);
void DrawTexture(Texture& texture, float x, float y, float width, float height, Color tint);
void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
                    float srcX, float srcY, float srcWidth, float srcHeight, Color tint);

void DrawRectangle(float x, float y, float w, float h, Color color);
void DrawCircle(float x, float y, float radius, Color color);
void DrawLine(float x1, float y1, float x2, float y2, Color color);

void DrawGlyph(unsigned int atlasID, float x, float y, float w, float h,
			   float u0, float v0, float u1, float v1, Color color);

void BeginCamera2D(const Camera2D& cam);
void EndCamera2D();

// -----------------------------------------------------------------------
// Debug mode — draws FPS, mouse crosshair, red border overlay
// -----------------------------------------------------------------------
void DebugMode(bool enabled);
bool IsDebugMode();
// Convert screen-space mouse position to world-space using a camera.
// Camera transform: world = cam.xy + (screen - halfScreen) / zoom
float GetWorldMouseX(const Camera2D& cam);
float GetWorldMouseY(const Camera2D& cam);
