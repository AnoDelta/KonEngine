#include "KonEngine.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

// -----------------------------------------------------------------------
// Simple pass/fail test runner
// -----------------------------------------------------------------------
static int s_passed = 0;
static int s_failed = 0;

#define TEST(name, expr) do { \
    if (expr) { \
        std::cout << "  [PASS] " << name << "\n"; \
        s_passed++; \
    } else { \
        std::cout << "  [FAIL] " << name << "\n"; \
        s_failed++; \
    } \
} while(0)

#define SECTION(name) std::cout << "\n-- " << name << " --\n"

// -----------------------------------------------------------------------
// Headless tests
// -----------------------------------------------------------------------

void test_curves() {
    SECTION("Curves");
    TEST("Linear(0.0) == 0.0", Curves::Linear(0.0f) == 0.0f);
    TEST("Linear(1.0) == 1.0", Curves::Linear(1.0f) == 1.0f);
    TEST("Linear(0.5) == 0.5", Curves::Linear(0.5f) == 0.5f);
    TEST("EaseIn(0.0) == 0.0",  Curves::EaseIn(0.0f) == 0.0f);
    TEST("EaseIn(1.0) == 1.0",  Curves::EaseIn(1.0f) == 1.0f);
    TEST("EaseOut(0.0) == 0.0", Curves::EaseOut(0.0f) == 0.0f);
    TEST("EaseOut(1.0) == 1.0", Curves::EaseOut(1.0f) == 1.0f);
    TEST("EaseInOut midpoint < 0.5", Curves::EaseIn(0.5f) < 0.5f);
    TEST("EaseOut midpoint > 0.5", Curves::EaseOut(0.5f) > 0.5f);
    TEST("Apply dispatches correctly",
         std::abs(Curves::Apply(Ease::Linear, 0.75f) - 0.75f) < 0.0001f);
}

void test_keyframe_track() {
    SECTION("KeyframeTrack sampling");
    KeyframeTrack track;
    track.AddKey(0.0f, 0.0f, Ease::Linear);
    track.AddKey(1.0f, 100.0f, Ease::Linear);

    TEST("Sample at 0.0 == 0",   std::abs(track.Sample(0.0f) - 0.0f)   < 0.01f);
    TEST("Sample at 1.0 == 100", std::abs(track.Sample(1.0f) - 100.0f) < 0.01f);
    TEST("Sample at 0.5 == 50",  std::abs(track.Sample(0.5f) - 50.0f)  < 0.01f);
    TEST("Sample before start",  std::abs(track.Sample(-1.0f) - 0.0f)  < 0.01f);
    TEST("Sample after end",     std::abs(track.Sample(2.0f) - 100.0f) < 0.01f);

    KeyframeTrack single;
    single.AddKey(0.5f, 42.0f);
    TEST("Single key always returns its value",
         std::abs(single.Sample(0.0f) - 42.0f) < 0.01f);
    TEST("Single key always returns its value (after)",
         std::abs(single.Sample(1.0f) - 42.0f) < 0.01f);
}

void test_animation_clip() {
    SECTION("Animation clip");
    Animation anim("walk", true);
    anim.AddFrame(0,  0, 32, 32, 0.1f);
    anim.AddFrame(32, 0, 32, 32, 0.1f);
    anim.AddFrame(64, 0, 32, 32, 0.1f);

    TEST("Frame count == 3", anim.frames.size() == 3);
    TEST("Duration == 0.3",  std::abs(anim.duration - 0.3f) < 0.001f);
    TEST("Loop flag set",    anim.loop == true);
    TEST("Name correct",     anim.name == "walk");

    anim.Track("x").AddKey(0.0f, 0.0f).AddKey(1.0f, 100.0f);
    anim.AutoDuration();
    TEST("AutoDuration extends to track end", anim.duration >= 1.0f);
}

void test_node_tree() {
    SECTION("Node tree");
    Node root("root");
    auto* child  = root.AddChild<Node>("child");
    auto* child2 = root.AddChild<Node>("child2");
    auto* grand  = child->AddChild<Node>("grandchild");

    TEST("child parent == root",       child->parent == &root);
    TEST("grandchild parent == child", grand->parent == child);
    TEST("GetNode finds child",        root.GetNode("child") == child);
    TEST("GetNode finds grandchild",   root.GetNode("grandchild") == grand);
    TEST("GetNode returns null for missing", root.GetNode("nope") == nullptr);

    int count = 0;
    root.ForEachDescendant([&](Node*) { count++; });
    TEST("ForEachDescendant visits 3 nodes", count == 3);

    root.RemoveChild("child2");
    count = 0;
    root.ForEachDescendant([&](Node*) { count++; });
    TEST("After RemoveChild, 2 nodes remain", count == 2);
}

void test_node2d() {
    SECTION("Node2D");
    Node2D n;
    n.x = 100; n.y = 200;
    n.originX = 0.5f; n.originY = 0.5f;

    TEST("DrawX centers at x - w*0.5", std::abs(n.DrawX(64) - 68.0f) < 0.01f);
    TEST("DrawY centers at y - h*0.5", std::abs(n.DrawY(32) - 184.0f) < 0.01f);

    n.originX = 0.0f; n.originY = 0.0f;
    TEST("DrawX top-left origin == x", std::abs(n.DrawX(64) - 100.0f) < 0.01f);
    TEST("DrawY top-left origin == y", std::abs(n.DrawY(64) - 200.0f) < 0.01f);

    n.Move(10, -5);
    TEST("Move adds to x", std::abs(n.x - 110.0f) < 0.01f);
    TEST("Move adds to y", std::abs(n.y - 195.0f) < 0.01f);
}

void test_signals() {
    SECTION("Signals");
    Node n("sig_test");
    int fired = 0;
    n.Connect("test_signal", [&]() { fired++; });
    n.Emit("test_signal");
    n.Emit("test_signal");
    TEST("Signal fired twice",           fired == 2);
    TEST("Unknown signal doesn't crash", (n.Emit("nonexistent"), true));
}

void test_animation_player_headless() {
    SECTION("AnimationPlayer (headless)");

    AnimationPlayer ap("test_ap");
    Animation clip("run", true);
    clip.AddFrame(0,  0, 32, 32, 0.1f);
    clip.AddFrame(32, 0, 32, 32, 0.1f);
    clip.displayW = 32; clip.displayH = 32; clip.displayScale = 1.0f;
    ap.Add(clip);

    TEST("IsPlaying() false before Play()", !ap.IsPlaying());
    TEST("IsFinished() false before Play()", !ap.IsFinished());

    ap.Play("run");
    TEST("IsPlaying() true after Play()", ap.IsPlaying());
    TEST("GetCurrent() == run", ap.GetCurrent() == "run");

    ap.Pause();
    TEST("IsPlaying() false after Pause()", !ap.IsPlaying());

    ap.Resume();
    TEST("IsPlaying() true after Resume()", ap.IsPlaying());

    ap.Stop();
    TEST("IsPlaying() false after Stop()", !ap.IsPlaying());
    TEST("IsFinished() true after Stop()", ap.IsFinished());

    ap.Play("nonexistent");
    TEST("Play unknown anim doesn't crash", true);
}

void test_collision() {
    SECTION("Collision (AABB)");
    Rectangle a(0, 0, 100, 100);
    Rectangle b(50, 50, 100, 100);
    Rectangle c(200, 200, 50, 50);

    TEST("Overlapping rects collide",     CheckCollisionRecs(a, b));
    TEST("Non-overlapping rects no col",  !CheckCollisionRecs(a, c));
    TEST("Touching edge does NOT collide (strict)", !CheckCollisionRecs(
             Rectangle(0,0,100,100), Rectangle(100,0,100,100)));
    TEST("1px overlap does collide", CheckCollisionRecs(
             Rectangle(0,0,100,100), Rectangle(99,0,100,100)));
}

void test_collision_world() {
    SECTION("CollisionWorld + Collider2D + signals");

    Collider2D a("a"), b("b"), c("c");
    a.x = 0;   a.y = 0;   a.width = 100; a.height = 100;
    b.x = 50;  b.y = 50;  b.width = 100; b.height = 100;
    c.x = 300; c.y = 300; c.width = 100; c.height = 100;

    TEST("Overlaps() detects overlap",    CollisionWorld::Overlaps(&a, &b));
    TEST("Overlaps() detects no overlap", !CollisionWorld::Overlaps(&a, &c));

    int enterCount = 0, exitCount = 0;
    a.Connect("on_collision_enter", [&](Collider2D*) { enterCount++; });
    a.Connect("on_collision_exit",  [&](Collider2D*) { exitCount++;  });

    CollisionWorld world;
    world.Add(&a);
    world.Add(&b);
    world.Add(&c);

    world.Update();
    TEST("collision_enter fired for overlap",   enterCount == 1);
    TEST("collision_exit not fired yet",        exitCount  == 0);

    world.Update();
    TEST("No duplicate enter on stay",          enterCount == 1);

    b.x = 500;
    world.Update();
    TEST("collision_exit fired after separation", exitCount == 1);

    Collider2D d("d"), e("e");
    d.x = 0; d.y = 0; d.width = 50; d.height = 50;
    e.x = 0; e.y = 0; e.width = 50; e.height = 50;
    d.layer = 1; d.mask = 2;
    e.layer = 4; e.mask = 4;
    TEST("Layer mask filters out non-matching",
         !((d.layer & e.mask) || (e.layer & d.mask)));

    Collider2D ca("ca"), cb("cb");
    ca.shape = ColliderShape::Circle; ca.x = 0;  ca.y = 0; ca.radius = 50;
    cb.shape = ColliderShape::Circle; cb.x = 60; cb.y = 0; cb.radius = 50;
    TEST("Circle vs circle overlap",    CollisionWorld::Overlaps(&ca, &cb));
    cb.x = 200;
    TEST("Circle vs circle no overlap", !CollisionWorld::Overlaps(&ca, &cb));
}

void test_debug_mode() {
    SECTION("DebugMode");
    DebugMode(false);
    TEST("IsDebugMode false when off", !IsDebugMode());
    DebugMode(true);
    TEST("IsDebugMode true when on", IsDebugMode());
    DebugMode(false);
    TEST("IsDebugMode false after disable", !IsDebugMode());
}

// -----------------------------------------------------------------------
// Visual / interactive tests
// -----------------------------------------------------------------------

void run_visual_tests() {
    SECTION("Visual / window tests (manual verification)");
    std::cout << "  Window will open. Verify each item visually, then close.\n";
    std::cout << "\n  Rendering checklist:\n";
    std::cout << "    [ ] Window opens and is responsive\n";
    std::cout << "    [ ] Background clears to dark grey each frame\n";
    std::cout << "    [ ] Green rectangle visible in center\n";
    std::cout << "    [ ] Red circle visible top-left\n";
    std::cout << "    [ ] Blue line drawn diagonally\n";
    std::cout << "    [ ] White rectangle moves with WASD\n";
    std::cout << "    [ ] Camera zoom animates smoothly\n";
    std::cout << "    [ ] FPS stays near 60\n";
    std::cout << "\n  Debug mode checklist:\n";
    std::cout << "    [ ] Red border around window\n";
    std::cout << "    [ ] Red crosshair follows mouse\n";
    std::cout << "    [ ] Two green collider outlines visible\n";
    std::cout << "    [ ] Colliders turn RED when overlapping\n";
    std::cout << "    [ ] Terminal prints Collision ENTER on overlap\n";
    std::cout << "    [ ] Terminal prints Collision EXIT on separate\n";
    std::cout << "\n  Input checklist:\n";
    std::cout << "    [ ] W/A/S/D moves the white rectangle\n";
    std::cout << "    [ ] SPACE prints SPACE pressed\n";
    std::cout << "    [ ] Left click prints position\n";
    std::cout << "    [ ] Right-click drag moves colB into colA\n";
    std::cout << "    [ ] ESC closes the window\n";
    std::cout << "\n  Press ESC or close window when done.\n\n";

    DebugMode(true);
    InitWindow(800, 600, "KonEngine -- Visual Test");
    SetTargetFPS(60);

    Scene scene;

    // Sprite2D — moves with WASD
    auto* s = scene.Add<Sprite2D>("test_sprite");
    s->x = 400; s->y = 300;
    s->width = 64; s->height = 64;
    s->tint = WHITE;

    // Two colliders for visual collision test
    auto* colA = scene.Add<Collider2D>("colA");
    colA->x = 300; colA->y = 300;
    colA->width = 80; colA->height = 80;
    colA->debugDraw = true;
    colA->debugColor = { 0.0f, 1.0f, 0.0f, 1.0f };

    auto* colB = scene.Add<Collider2D>("colB");
    colB->x = 500; colB->y = 300;
    colB->width = 80; colB->height = 80;
    colB->debugDraw = true;
    colB->debugColor = { 0.0f, 1.0f, 0.0f, 1.0f };

    colA->Connect("on_collision_enter", [&](Collider2D*) {
        std::cout << "  [VISUAL] Collision ENTER!\n";
        colA->debugColor = { 1.0f, 0.0f, 0.0f, 1.0f };
        colB->debugColor = { 1.0f, 0.0f, 0.0f, 1.0f };
    });
    colA->Connect("on_collision_exit", [&](Collider2D*) {
        std::cout << "  [VISUAL] Collision EXIT!\n";
        colA->debugColor = { 0.0f, 1.0f, 0.0f, 1.0f };
        colB->debugColor = { 0.0f, 1.0f, 0.0f, 1.0f };
    });

    Camera2D cam(400, 300, 2.0f, 0.0f);
    static float elapsed = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();
        elapsed += dt;

        // Animate camera zoom gently
        float zoom = 2.0f + std::sin(elapsed) * 0.3f;
        cam = Camera2D(400, 300, zoom, 0.0f);

        // WASD moves sprite
        if (IsKeyDown(Key::W)) s->y -= 200.0f * dt;
        if (IsKeyDown(Key::S)) s->y += 200.0f * dt;
        if (IsKeyDown(Key::A)) s->x -= 200.0f * dt;
        if (IsKeyDown(Key::D)) s->x += 200.0f * dt;

        // Right-click drag moves colB — divide by zoom to get world coords
        if (IsMouseButtonDown(Mouse::Right)) {
            colB->x = 400.0f + (GetMouseX() - 400.0f) / zoom;
            colB->y = 300.0f + (GetMouseY() - 300.0f) / zoom;
        }

        if (IsKeyPressed(Key::Space))
            std::cout << "  SPACE pressed\n";
        if (IsKeyPressed(Key::Escape))
            break;

        if (IsMouseButtonPressed(Mouse::Left))
            std::cout << "  LEFT click at ("
                      << (int)GetMouseX() << ", "
                      << (int)GetMouseY() << ")\n";
        if (IsMouseButtonPressed(Mouse::Right))
            std::cout << "  RIGHT click at ("
                      << (int)GetMouseX() << ", "
                      << (int)GetMouseY() << ")\n";

        // FPS every second
        static float fpsTimer = 0.0f;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << (int)(1.0f / dt) << "\n";
            fpsTimer = 0.0f;
        }

        ClearBackground(0.12f, 0.12f, 0.12f);

        // Raw draw calls (screen space, no camera)
        DrawRectangle(350, 250, 100, 100, GREEN);
        DrawCircle(100, 100, 40, RED);
        DrawLine(0, 0, 800, 600, BLUE);

        // Scene (world space, with camera)
        BeginCamera2D(cam);
        scene.Update(dt);
        scene.Draw();
        EndCamera2D();

        Present();
        PollEvents();
    }
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main() {
    std::cout << "========================================\n";
    std::cout << "  KonEngine Test Suite\n";
    std::cout << "========================================\n";

    test_curves();
    test_keyframe_track();
    test_animation_clip();
    test_node_tree();
    test_node2d();
    test_signals();
    test_animation_player_headless();
    test_collision();
    test_collision_world();
    test_debug_mode();

    std::cout << "\n========================================\n";
    std::cout << "  Headless results: "
              << s_passed << " passed, "
              << s_failed << " failed\n";
    std::cout << "========================================\n";

    if (s_failed > 0) {
        std::cout << "\nFailing tests found -- fix before release!\n";
        return 1;
    }

    run_visual_tests();

    std::cout << "\n========================================\n";
    std::cout << "  All headless tests passed.\n";
    std::cout << "  Verify visual checklist above.\n";
    std::cout << "========================================\n";

    return 0;
}
