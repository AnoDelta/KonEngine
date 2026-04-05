// KonEngine — Pure C++ Example (no KonScript)
// Build: mkdir build && cd build && cmake .. && make && ./game

#include "KonEngine.hpp"

class Player : public Sprite2D {
public:
    float speed = 200.0f;

    Player(const std::string& name = "Player") : Sprite2D(name) {
        width = 32; height = 48;
        tint = {0.2f, 0.8f, 1.0f, 1.0f};
    }

    void Update(float dt) override {
        if (IsKeyDown(Key::A) || IsKeyDown(Key::Left))  x -= speed * dt;
        if (IsKeyDown(Key::D) || IsKeyDown(Key::Right)) x += speed * dt;
        if (IsKeyDown(Key::W) || IsKeyDown(Key::Up))    y -= speed * dt;
        if (IsKeyDown(Key::S) || IsKeyDown(Key::Down))  y += speed * dt;
    }
};

int main() {
    InitWindow(800, 600, "KonEngine C++ Example");
    SetTargetFPS(60);

    Scene scene;
    auto* player = scene.Add<Player>("player");
    player->x = 400;
    player->y = 300;

    while (!WindowShouldClose()) {
        scene.Update(GetDeltaTime());
        ClearBackground(0.1f, 0.1f, 0.15f);
        scene.Draw();
        Present();
        PollEvents();
    }

    return 0;
}
