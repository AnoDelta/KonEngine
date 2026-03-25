#include "KonEngine.hpp"
#include "window/window.hpp"
#include <iostream>

int main() {

	const int width = 500, height = 500;

	const int rectWidth = width / 3; 
	const int rectHeight = height / 3;

	int rectX = static_cast<int>(width / 2 - rectWidth / 2);
	int rectY = static_cast<int>(height / 2 - rectHeight / 2);

	InitWindow(width, height, "KonAkiEngine");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		ClearBackground(0.2f, 0.0f, 0.0f);
		DrawRectangle(rectX, rectY, rectWidth, rectHeight, 1, 1, 1);
		rectX++;

		// Prints fps
		std::cout << "FPS: " << 1.0f / GetDeltaTime() << '\n';
		Present();
		PollEvents();
	}

	return 0;
}
