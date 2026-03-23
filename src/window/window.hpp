#pragma once

#include <string>
#include <memory>

class Window {
public:
	Window(int width, int height, const std::string& title);
	~Window();

	void pollEvents();
	bool shouldClose() const;
	void swapBuffers();

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

void InitWindow(int width, int height, const std::string& title);
bool WindowShouldClose();
void Present();
void PollEvents();
