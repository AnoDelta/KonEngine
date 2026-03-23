#include "KonEngine.hpp"

int main() {
	InitWindow(900,  800, "KonAkiEngine");

	while (!WindowShouldClose()) {
		ClearBackground(0.2f, 0.3f, 0.3f);
		Present();
		PollEvents();
	}

	return 0;
}
