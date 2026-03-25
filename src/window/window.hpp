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
