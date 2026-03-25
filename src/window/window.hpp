#pragma once

#include <string>
#include <memory>
#include "../renderer/renderer.hpp"

class Window {
public:
	Window(int width, int height, const std::string& title);
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

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	std::unique_ptr<IRenderer> renderer;
};

void InitWindow(int width, int height, const std::string& title);
bool WindowShouldClose();
void Present();
void PollEvents();
void ClearBackground(float r, float g, float b);

int GetWindowWidth();
int GetWindowHeight();

void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0f);
