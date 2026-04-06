// KonEngine — Pure C++ Example (no KonScript)
// Showcases: Color presets, Camera2D, collision, scene tree, drawing, input,
//            KinematicBody2D, StaticBody2D, text, debug mode
//
// Build: mkdir build && cd build && cmake .. && make && ./game
//        (or use the engine as a subdirectory — see CMakeLists.txt)

#include "KonEngine.hpp"

// ---------------------------------------------------------------------------
// Player — WASD movement with wall collision
// ---------------------------------------------------------------------------
class Player : public KinematicBody2D {
public:
    float speed = 200.0f;

    Player(const std::string& name = "Player") : KinematicBody2D(name) {}

    void Ready() override {
        AddCollider(32, 48);   // auto-names "collider_0"
        KinematicBody2D::Ready();
    }

    void Update(float dt) override {
        float dx = 0, dy = 0;
        if (IsKeyDown(Key::A) || IsKeyDown(Key::Left))  dx -= speed * dt;
        if (IsKeyDown(Key::D) || IsKeyDown(Key::Right)) dx += speed * dt;
        if (IsKeyDown(Key::W) || IsKeyDown(Key::Up))    dy -= speed * dt;
        if (IsKeyDown(Key::S) || IsKeyDown(Key::Down))  dy += speed * dt;
        MoveAndCollide(dx, dy);  // no need to pass CollisionWorld
    }

    void OnCollisionEnter(Collider2D* other) override {
        tint = RED;
    }

    void OnCollisionExit(Collider2D* other) override {
        tint = CYAN;
    }

    void Draw() override {
        DrawRectangle(DrawX(32), DrawY(48), 32, 48, tint);
    }
};

// ---------------------------------------------------------------------------
// Wall — static obstacle the player collides with
// ---------------------------------------------------------------------------
class Wall : public StaticBody2D {
public:
    float w, h;
    Color color = GRAY;

    Wall(const std::string& name, float w = 120, float h = 24)
        : StaticBody2D(name), w(w), h(h) {}

    void Ready() override {
        AddCollider(w, h);
        StaticBody2D::Ready();
    }

    void Draw() override {
        DrawRectangle(DrawX(w), DrawY(h), w, h, color);
    }
};

// ---------------------------------------------------------------------------
// Coin — simple yellow circle
// ---------------------------------------------------------------------------
class Coin : public Node2D {
public:
    float radius = 10.0f;

    Coin(const std::string& name = "Coin") : Node2D(name) {}

    void Draw() override {
        DrawCircle(x, y, radius, YELLOW);
    }
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    // Pass true for resizable window with letterbox scaling
    InitWindow(800, 600, "KonEngine C++ Example", true);
    SetTargetFPS(60);
    DebugMode(true);   // FPS overlay, collider outlines, debug grid

    Scene scene;

    // Player at center of the world
    auto* player = scene.Add<Player>("player");
    player->x = 400;
    player->y = 300;
    player->tint = CYAN;

    // Walls around the edges
    auto* floor = scene.Add<Wall>("floor", 600, 20);
    floor->x = 400; floor->y = 510;

    auto* wallL = scene.Add<Wall>("wallL", 20, 400);
    wallL->x = 110; wallL->y = 300;

    auto* wallR = scene.Add<Wall>("wallR", 20, 400);
    wallR->x = 690; wallR->y = 300;

    auto* ceiling = scene.Add<Wall>("ceiling", 600, 20);
    ceiling->x = 400; ceiling->y = 110;

    auto* platform = scene.Add<Wall>("platform", 160, 16);
    platform->x = 400; platform->y = 400;
    platform->color = ORANGE;

    // Scatter some coins
    const float coinPos[][2] = {
        {250, 200}, {550, 200}, {400, 350}, {200, 430}, {600, 430}
    };
    for (int i = 0; i < 5; i++) {
        auto* coin = scene.Add<Coin>("coin" + std::to_string(i));
        coin->x = coinPos[i][0];
        coin->y = coinPos[i][1];
    }

    // Camera follows the player with smooth interpolation
    Camera2D cam(player->x, player->y, 1.0f);
    float targetZoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();

        // Update all nodes + collision detection
        scene.Update(dt);

        // Camera zoom via scroll wheel
        float scroll = GetMouseScroll();
        if (scroll != 0.0f) {
            targetZoom += scroll * 0.1f;
            if (targetZoom < 0.3f) targetZoom = 0.3f;
            if (targetZoom > 3.0f) targetZoom = 3.0f;
        }
        cam.zoom += (targetZoom - cam.zoom) * 5.0f * dt;

        // Smooth camera follow
        Camera2DFollow(cam, player->x, player->y, 0.4f, dt);

        // --- Render ---

        // Clear with a Color instead of individual r, g, b
        ClearBackground(BLACK);

        // World-space drawing (affected by camera)
        BeginCamera2D(cam);
            scene.Draw();

            // Draw a line from player to mouse cursor (world coords)
            float wmx = GetWorldMouseX(cam);
            float wmy = GetWorldMouseY(cam);
            DrawLine(player->x, player->y, wmx, wmy, MAGENTA);
        EndCamera2D();

        // Screen-space HUD (not affected by camera)
        DrawRectangle(10, 560, 260, 30, Color(0.0f, 0.0f, 0.0f, 0.6f));
        DrawText("WASD: move | Scroll: zoom | ESC: quit", 16, 566, WHITE);

        Present();
        PollEvents();

        if (IsKeyPressed(Key::Escape)) break;
    }

    return 0;
}
