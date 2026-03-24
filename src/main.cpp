#include "KonEngine.hpp"
#include <iostream>

int main() {
	InitWindow(900,  800, "KonAkiEngine");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		ClearBackground(0.2f, 0.0f, 0.0f);
		std::cout << "FPS: " << 1.0f / GetDeltaTime() << '\n';
		Present();
		PollEvents();
	}

	return 0;
}
