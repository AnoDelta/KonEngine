#include "window/window.hpp"

int main() {
	InitWindow(900,  800, "This is the first KonAki Game Engine Test");

	while (!WindowShouldClose()) {
		Present();
		PollEvents();
	}

	return 0;
}
