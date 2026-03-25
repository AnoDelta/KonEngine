#include <KonEngine>
#include "window/window.hpp"
#include <iostream>

int main() {

	const int width = 500, height = 500;

	const int rectWidth = width / 3; 
	const int rectHeight = height / 3;

	int rectX = static_cast<int>(width / 2 - rectWidth / 2);
	int rectY = static_cast<int>(height / 2 - rectHeight / 2);

	float speed = 3;

	int directionX = 1;
	int directionY = 1;

	InitWindow(width, height, "KonAkiEngine");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		ClearBackground(0.2f, 0.0f, 0.0f);
		DrawRectangle(rectX, rectY, rectWidth, rectHeight, 1, 1, 1);

		if (rectX <= 0 || rectX >= width - rectWidth) {
			directionX = -directionX;
		}

		if (rectY <= 0 || rectY >= height - rectHeight) {
			directionY = -directionY;
		}

		rectX += static_cast<int>(directionX * speed);
		rectY += static_cast<int>(directionY * speed);

		Present();
		PollEvents();

		// Prints fps
		std::cout << "FPS: " << 1.0f / GetDeltaTime() << '\n';
	}

	return 0;
}
