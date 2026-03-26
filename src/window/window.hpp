#pragma once

#include <string>
#include <memory>
#include "../renderer/renderer.hpp"
#include "../color/color.hpp"

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
	int getHeight();
	int getWidth();
	void setVsync(bool enabled);

	unsigned int loadTexture(const char* path);
	void unloadTexture(unsigned int id);
	void drawTexture(unsigned int id, float x, float y, float width, float height);
	void drawTextureRec(unsigned int id, float x, float y, float width, float height,
					 float srcX, float srcY, float srcWidth, float srcHeight);

	void drawRectangle(float x, float y, float w, float h, Color color);
	void drawCircle(float x, float y, float radius, Color color);
	void drawLine(float x1, float y1, float x2, float y2, Color color);
	void drawTexture(unsigned int id, float x, float y, float w, float h, Color tint);
	void drawTextureRec(unsigned int id, float x, float y, float w, float h,
						float srcX, float srcY, float srcW, float srcH, Color tint);

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

unsigned int LoadTexture(const char* path);
void UnloadTexture(unsigned int id);
void DrawTexture(unsigned int id, float x, float y, float width, float height);
void DrawTextureRec(unsigned int id, float x, float y, float width, float height,
                    float srcX, float srcY, float srcWidth, float srcHeight);

void DrawRectangle(float x, float y, float w, float h, Color color);
void DrawCircle(float x, float y, float radius, Color color);
void DrawLine(float x1, float y1, float x2, float y2, Color color);
void DrawTexture(unsigned int id, float x, float y, float w, float h, Color tint);
void DrawTextureRec(unsigned int id, float x, float y, float w, float h,
					float srcX, float srcY, float srcW, float srcH, Color tint);
