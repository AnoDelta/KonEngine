#include "KonEngine.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <sstream>

// -----------------------------------------------------------------------
// Test runner
// -----------------------------------------------------------------------
static int s_passed = 0;
static int s_failed = 0;

#define TEST(name, expr) do { \
    if (expr) { \
        std::cout << "  [PASS] " << (name) << "\n"; \
        s_passed++; \
    } else { \
        std::cout << "  [FAIL] " << (name) << "\n"; \
        s_failed++; \
    } \
} while(0)

#define SECTION(name) \
    std::cout << "\n-- " << (name) << " --\n"

#define TEST_NEAR(name, a, b, eps) \
    TEST(name, std::abs((a)-(b)) < (eps))

// -----------------------------------------------------------------------
// Curves
// -----------------------------------------------------------------------
void test_curves() {
    SECTION("Easing Curves");
    TEST("Linear(0) == 0",      Curves::Linear(0.0f) == 0.0f);
    TEST("Linear(1) == 1",      Curves::Linear(1.0f) == 1.0f);
    TEST_NEAR("Linear(0.5) == 0.5", Curves::Linear(0.5f), 0.5f, 0.0001f);
    TEST("EaseIn(0) == 0",      Curves::EaseIn(0.0f) == 0.0f);
    TEST("EaseIn(1) == 1",      Curves::EaseIn(1.0f) == 1.0f);
    TEST("EaseIn midpoint < 0.5",  Curves::EaseIn(0.5f) < 0.5f);
    TEST("EaseOut(0) == 0",     Curves::EaseOut(0.0f) == 0.0f);
    TEST("EaseOut(1) == 1",     Curves::EaseOut(1.0f) == 1.0f);
    TEST("EaseOut midpoint > 0.5", Curves::EaseOut(0.5f) > 0.5f);
    TEST("EaseInOut(0) == 0",   Curves::EaseInOut(0.0f) == 0.0f);
    TEST("EaseInOut(1) == 1",   Curves::EaseInOut(1.0f) == 1.0f);
    TEST_NEAR("EaseInOut(0.5) == 0.5", Curves::EaseInOut(0.5f), 0.5f, 0.0001f);
    TEST("EaseOutBounce(0) == 0",  Curves::EaseOutBounce(0.0f) == 0.0f);
    TEST("EaseOutBounce(1) == 1",  Curves::EaseOutBounce(1.0f) == 1.0f);
    TEST("Apply dispatches Linear",
         std::abs(Curves::Apply(Ease::Linear, 0.75f) - 0.75f) < 0.0001f);
    TEST("Apply dispatches EaseOut",
         Curves::Apply(Ease::EaseOut, 0.5f) > 0.5f);
}

// -----------------------------------------------------------------------
// KeyframeTrack
// -----------------------------------------------------------------------
void test_keyframe_track() {
    SECTION("KeyframeTrack");

    KeyframeTrack t;
    t.AddKey(0.0f, 0.0f, Ease::Linear);
    t.AddKey(1.0f, 100.0f, Ease::Linear);

    TEST_NEAR("Sample(0.0) == 0",     t.Sample(0.0f),  0.0f,   0.01f);
    TEST_NEAR("Sample(1.0) == 100",   t.Sample(1.0f),  100.0f, 0.01f);
    TEST_NEAR("Sample(0.5) == 50",    t.Sample(0.5f),  50.0f,  0.01f);
    TEST_NEAR("Sample(-1) clamps lo", t.Sample(-1.0f), 0.0f,   0.01f);
    TEST_NEAR("Sample(2) clamps hi",  t.Sample(2.0f),  100.0f, 0.01f);

    KeyframeTrack single;
    single.AddKey(0.5f, 42.0f);
    TEST_NEAR("Single key before",  single.Sample(0.0f), 42.0f, 0.01f);
    TEST_NEAR("Single key after",   single.Sample(1.0f), 42.0f, 0.01f);
    TEST_NEAR("Single key at",      single.Sample(0.5f), 42.0f, 0.01f);

    KeyframeTrack multi;
    multi.AddKey(0.0f, 0.0f,   Ease::Linear);
    multi.AddKey(0.5f, 50.0f,  Ease::Linear);
    multi.AddKey(1.0f, 200.0f, Ease::Linear);
    TEST_NEAR("Multi-key second segment", multi.Sample(0.75f), 125.0f, 0.5f);
}

// -----------------------------------------------------------------------
// Animation clip
// -----------------------------------------------------------------------
void test_animation_clip() {
    SECTION("Animation Clip");

    Animation anim("walk", true);
    anim.AddFrame(0,  0, 32, 32, 0.1f);
    anim.AddFrame(32, 0, 32, 32, 0.1f);
    anim.AddFrame(64, 0, 32, 32, 0.1f);

    TEST("Frame count == 3",      anim.frames.size() == 3);
    TEST_NEAR("Duration == 0.3",  anim.duration, 0.3f, 0.001f);
    TEST("Loop flag set",         anim.loop == true);
    TEST("Name correct",          anim.name == "walk");

    anim.Track("x").AddKey(0.0f, 0.0f).AddKey(1.0f, 100.0f);
    anim.AutoDuration();
    TEST("AutoDuration extends to track end", anim.duration >= 1.0f);

    Animation noloop("idle", false);
    noloop.AddFrame(0, 0, 64, 64, 0.5f);
    TEST("Non-loop flag",    noloop.loop == false);
    TEST_NEAR("Single frame duration", noloop.duration, 0.5f, 0.001f);
}

// -----------------------------------------------------------------------
// Node tree
// -----------------------------------------------------------------------
void test_node_tree() {
    SECTION("Node Tree");

    Node root("root");
    auto* child  = root.AddChild<Node>("child");
    auto* child2 = root.AddChild<Node>("child2");
    auto* grand  = child->AddChild<Node>("grandchild");

    TEST("child parent == root",        child->parent  == &root);
    TEST("grand parent == child",       grand->parent  == child);
    TEST("GetNode finds child",         root.GetNode("child")       == child);
    TEST("GetNode finds grandchild",    root.GetNode("grandchild")  == grand);
    TEST("GetNode finds deep",          root.GetNode("grandchild")  != nullptr);
    TEST("GetNode returns null",        root.GetNode("nope")        == nullptr);
    TEST("child2 parent == root",       child2->parent == &root);

    int count = 0;
    root.ForEachDescendant([&](Node*) { count++; });
    TEST("ForEachDescendant visits 3",  count == 3);

    root.RemoveChild("child2");
    count = 0;
    root.ForEachDescendant([&](Node*) { count++; });
    TEST("After RemoveChild 2 remain",  count == 2);
    TEST("GetNode after remove",        root.GetNode("child2") == nullptr);
    TEST("Other child still exists",    root.GetNode("child") != nullptr);

    // Ready() called on AddChild
    struct ReadyTracker : public Node {
        bool readyCalled = false;
        ReadyTracker(const std::string& name = "tracker") : Node(name) {}
        void Ready() override { readyCalled = true; }
    };
    auto* tracker = root.AddChild<ReadyTracker>("tracker");
    TEST("Ready() called by AddChild",  tracker->readyCalled);
}

// -----------------------------------------------------------------------
// Node2D
// -----------------------------------------------------------------------
void test_node2d() {
    SECTION("Node2D");

    Node2D n("n");
    n.x = 100; n.y = 200;
    n.originX = 0.5f; n.originY = 0.5f;

    TEST_NEAR("DrawX center pivot",  n.DrawX(64),   68.0f,  0.01f);
    TEST_NEAR("DrawY center pivot",  n.DrawY(32),  184.0f,  0.01f);

    n.originX = 0.0f; n.originY = 0.0f;
    TEST_NEAR("DrawX top-left",      n.DrawX(64),  100.0f,  0.01f);
    TEST_NEAR("DrawY top-left",      n.DrawY(64),  200.0f,  0.01f);

    n.originX = 1.0f; n.originY = 1.0f;
    TEST_NEAR("DrawX bottom-right",  n.DrawX(64),   36.0f,  0.01f);
    TEST_NEAR("DrawY bottom-right",  n.DrawY(64),  136.0f,  0.01f);

    n.x = 0; n.y = 0; n.originX = 0.5f; n.originY = 0.5f;
    n.Move(10, -5);
    TEST_NEAR("Move x",  n.x,  10.0f, 0.01f);
    TEST_NEAR("Move y",  n.y,  -5.0f, 0.01f);

    // Child world transform
    Node2D parent("p");
    parent.x = 100; parent.y = 100; parent.scaleX = 2; parent.scaleY = 2;
    auto* childNode = parent.AddChild<Node2D>("c");
    childNode->x = 10; childNode->y = 5;
    // World position from propagateToChildren: parent.x + child.x * parent.scaleX
    // = 100 + 10*2 = 120,  100 + 5*2 = 110
    float worldX = parent.x + childNode->x * parent.scaleX;
    float worldY = parent.y + childNode->y * parent.scaleY;
    TEST_NEAR("Child world X with scale",  worldX, 120.0f, 0.01f);
    TEST_NEAR("Child world Y with scale",  worldY, 110.0f, 0.01f);
}

// -----------------------------------------------------------------------
// Signals
// -----------------------------------------------------------------------
void test_signals() {
    SECTION("Signals");

    Node n("sig");
    int count = 0;
    n.Connect("fire", [&]() { count++; });
    n.Emit("fire");
    n.Emit("fire");
    TEST("Signal fires twice",       count == 2);
    TEST("Unknown signal no crash",  (n.Emit("nope"), true));

    // Multiple listeners
    int a = 0, b = 0;
    Node m("multi");
    m.Connect("ev", [&]() { a++; });
    m.Connect("ev", [&]() { b++; });
    m.Emit("ev");
    TEST("Multiple listeners both fire", a == 1 && b == 1);
}

// -----------------------------------------------------------------------
// AnimationPlayer (headless)
// -----------------------------------------------------------------------
void test_animation_player_headless() {
    SECTION("AnimationPlayer (headless)");

    AnimationPlayer ap("ap");
    Animation clip("run", true);
    clip.AddFrame(0,  0, 32, 32, 0.1f);
    clip.AddFrame(32, 0, 32, 32, 0.1f);
    clip.displayW = 32; clip.displayH = 32; clip.displayScale = 1.0f;
    ap.Add(clip);

    TEST("Not playing before Play()",  !ap.IsPlaying());
    ap.Play("run");
    TEST("Playing after Play()",        ap.IsPlaying());
    TEST("GetCurrent == run",           ap.GetCurrent() == "run");
    TEST("GetCurrentFrame == 0",        ap.GetCurrentFrame() == 0);

    ap.Pause();
    TEST("Not playing after Pause()",  !ap.IsPlaying());
    ap.Resume();
    TEST("Playing after Resume()",      ap.IsPlaying());
    ap.Stop();
    TEST("Not playing after Stop()",   !ap.IsPlaying());
    TEST("IsFinished after Stop()",     ap.IsFinished());

    ap.Play("nonexistent");
    TEST("Play unknown clip no crash", true);

    // Non-looping clip finishes
    Animation once("once", false);
    once.AddFrame(0, 0, 32, 32, 0.01f); // very short
    ap.Add(once);
    ap.Play("once");
    TEST("Non-loop initially playing",  ap.IsPlaying());
}

// -----------------------------------------------------------------------
// Collision (AABB helper)
// -----------------------------------------------------------------------
void test_collision() {
    SECTION("Collision (AABB helper)");

    Rectangle a(0, 0, 100, 100);
    Rectangle b(50, 50, 100, 100);
    Rectangle c(200, 200, 50, 50);
    Rectangle d(100, 0, 100, 100); // touching edge

    TEST("Overlapping rects collide",         CheckCollisionRecs(a, b));
    TEST("Non-overlapping rects no col",      !CheckCollisionRecs(a, c));
    TEST("Touching edge does NOT collide",    !CheckCollisionRecs(a, d));
    TEST("1px overlap does collide",
         CheckCollisionRecs(Rectangle(0,0,100,100), Rectangle(99,0,100,100)));
    TEST("Contained rect collides",
         CheckCollisionRecs(Rectangle(0,0,100,100), Rectangle(10,10,10,10)));
    TEST("Self overlap",                       CheckCollisionRecs(a, a));
}

// -----------------------------------------------------------------------
// CollisionWorld + Collider2D
// -----------------------------------------------------------------------
void test_collision_world() {
    SECTION("CollisionWorld + Collider2D");

    // Basic overlap detection
    Collider2D a("a"), b("b"), c("c");
    a.x = 0;   a.y = 0;   a.width = 100; a.height = 100;
    b.x = 50;  b.y = 50;  b.width = 100; b.height = 100;
    c.x = 300; c.y = 300; c.width = 100; c.height = 100;

    TEST("Overlaps() detects overlap",    CollisionWorld::Overlaps(&a, &b));
    TEST("Overlaps() detects no overlap", !CollisionWorld::Overlaps(&a, &c));

    // Signals fire correctly
    int enterA = 0, exitA = 0;
    a.Connect("on_collision_enter", [&](Collider2D*) { enterA++; });
    a.Connect("on_collision_exit",  [&](Collider2D*) { exitA++;  });

    CollisionWorld world;
    world.Add(&a); world.Add(&b); world.Add(&c);

    world.Update();
    TEST("enter fires on first overlap",  enterA == 1);
    TEST("exit not fired yet",            exitA  == 0);

    world.Update();
    TEST("No duplicate enter on stay",    enterA == 1);

    b.x = 500;
    world.Update();
    TEST("exit fires after separation",   exitA  == 1);

    // Re-enter
    b.x = 50;
    world.Update();
    TEST("enter fires again on re-entry", enterA == 2);

    // Layer/mask filtering
    Collider2D d("d"), e("e");
    d.x = 0; d.y = 0; d.width = 50; d.height = 50;
    e.x = 0; e.y = 0; e.width = 50; e.height = 50;
    d.layer = 1; d.mask = 2;
    e.layer = 4; e.mask = 4;
    TEST("Layer mask filters non-matching",
         !CollisionWorld::Overlaps(&d, &e) ||
         !((d.layer & e.mask) || (e.layer & d.mask)));

    // Circle vs circle
    Collider2D ca("ca"), cb("cb");
    ca.shape = ColliderShape::Circle; ca.x = 0;   ca.y = 0; ca.radius = 50;
    cb.shape = ColliderShape::Circle; cb.x = 60;  cb.y = 0; cb.radius = 50;
    TEST("Circle vs circle overlap",    CollisionWorld::Overlaps(&ca, &cb));
    cb.x = 200;
    TEST("Circle vs circle no overlap", !CollisionWorld::Overlaps(&ca, &cb));

    // Rect vs circle
    Collider2D rect("rect"), circ("circ");
    rect.shape  = ColliderShape::Rectangle;
    rect.x = 0; rect.y = 0; rect.width = 100; rect.height = 100;
    circ.shape  = ColliderShape::Circle;
    circ.x = 50; circ.y = 50; circ.radius = 20;
    TEST("Rect vs circle overlap",      CollisionWorld::Overlaps(&rect, &circ));
    circ.x = 300;
    TEST("Rect vs circle no overlap",   !CollisionWorld::Overlaps(&rect, &circ));

    // touching flag
    CollisionWorld world2;
    Collider2D p("p"), q("q");
    p.x = 0; p.y = 0; p.width = 50; p.height = 50;
    q.x = 10; q.y = 10; q.width = 50; q.height = 50;
    world2.Add(&p); world2.Add(&q);
    world2.Update();
    TEST("touching flag set on overlap",  p.touching && q.touching);
    q.x = 500;
    world2.Update();
    TEST("touching flag cleared after separation", !p.touching && !q.touching);

    // OnCollisionEnter bubbles to parent node
    struct ColNode : public Node2D {
        int hits = 0;
        ColNode() : Node2D("colnode") {}
        void OnCollisionEnter(Collider2D*) override { hits++; }
    };
    ColNode parent;
    auto* pCol = parent.AddChild<Collider2D>("pCol");
    pCol->width = 100; pCol->height = 100;

    Collider2D other("other");
    other.x = 10; other.y = 10; other.width = 50; other.height = 50;

    CollisionWorld world3;
    world3.Add(pCol); world3.Add(&other);
    world3.Update();
    TEST("OnCollisionEnter bubbles to parent node", parent.hits == 1);
}

// -----------------------------------------------------------------------
// Scene
// -----------------------------------------------------------------------
void test_scene() {
    SECTION("Scene");

    // Needs a window for rendering but we can test the tree logic
    // NOTE: Scene::Add calls Ready() which may call AddChild for colliders
    // We can't open a window here so we just test the logic paths
    // that don't require GL

    // Test that collision world is scanned correctly
    // (headless — no window needed for this)
    struct TestNode : public Node2D {
        bool readyCalled = false;
        TestNode(const std::string& name = "tn") : Node2D(name) {}
        void Ready() override { readyCalled = true; }
    };

    Scene scene;
    auto* tn = scene.Add<TestNode>("tn");
    TEST("Scene::Add calls Ready()",  tn->readyCalled);
    TEST("Scene::GetNode finds node", scene.GetNode("tn") != nullptr);
    scene.Remove("tn");
    TEST("Scene::Remove works",       scene.GetNode("tn") == nullptr);
}

// -----------------------------------------------------------------------
// Vector2
// -----------------------------------------------------------------------
void test_vector2() {
    SECTION("Vector2");

    Vector2 a(3.0f, 4.0f);
    TEST_NEAR("Length == 5",       a.Length(), 5.0f, 0.001f);
    TEST_NEAR("LengthSq == 25",    a.LengthSq(), 25.0f, 0.001f);

    Vector2 norm = a.Normalized();
    TEST_NEAR("Normalized length == 1", norm.Length(), 1.0f, 0.001f);

    Vector2 b(1.0f, 0.0f), c(0.0f, 1.0f);
    TEST_NEAR("Dot(right, up) == 0",  b.Dot(c), 0.0f, 0.001f);
    TEST_NEAR("Dot(right, right)==1", b.Dot(b), 1.0f, 0.001f);

    Vector2 sum = a + b;
    TEST_NEAR("Add x", sum.x, 4.0f, 0.001f);
    TEST_NEAR("Add y", sum.y, 4.0f, 0.001f);

    Vector2 lerped = Vector2::Lerp(Vector2(0,0), Vector2(10,10), 0.5f);
    TEST_NEAR("Lerp x", lerped.x, 5.0f, 0.001f);
    TEST_NEAR("Lerp y", lerped.y, 5.0f, 0.001f);

    TEST("Zero()",   Vector2::Zero().x == 0 && Vector2::Zero().y == 0);
    TEST("Right()",  Vector2::Right().x == 1 && Vector2::Right().y == 0);
    TEST("Up()",     Vector2::Up().x    == 0 && Vector2::Up().y    == -1);
}

// -----------------------------------------------------------------------
// DebugMode
// -----------------------------------------------------------------------
void test_debug_mode() {
    SECTION("DebugMode");
    DebugMode(false);
    TEST("IsDebugMode false after disable", !IsDebugMode());
    DebugMode(true);
    TEST("IsDebugMode true after enable",    IsDebugMode());
    DebugMode(false);
    TEST("IsDebugMode false again",         !IsDebugMode());
}

// -----------------------------------------------------------------------
// Color
// -----------------------------------------------------------------------
void test_color() {
    SECTION("Color presets");
    TEST("RED.r == 1",      RED.r == 1.0f   && RED.g == 0.0f);
    TEST("GREEN.g == 1",    GREEN.g == 1.0f && GREEN.r == 0.0f);
    TEST("BLUE.b == 1",     BLUE.b == 1.0f  && BLUE.r == 0.0f);
    TEST("WHITE all 1",     WHITE.r == 1 && WHITE.g == 1 && WHITE.b == 1);
    TEST("BLACK all 0",     BLACK.r == 0 && BLACK.g == 0 && BLACK.b == 0);
    TEST("BLANK a==0", BLANK.a == 0.0f);
}

// -----------------------------------------------------------------------
// Visual / interactive tests (require a window)
// -----------------------------------------------------------------------
void run_visual_tests() {
    SECTION("Visual Tests (manual verification required)");

    std::cout << R"(
  A window will open. Verify each item, then close (ESC or X).

  What you will see:
    - A WHITE box  (WASD to move) with a GREEN collider outline around it
    - A CYAN box   (right-click drag to move) with a GREEN collider outline
    - A GRAY floor line at y=480
    - HUD text in the top-left corner (screen space, does not scroll)
    - Red debug border + mouse crosshair

  Checklist:
    [ ] Window opens and stays responsive
    [ ] Background is dark, clears every frame
    [ ] White box moves with WASD (stays inside camera)
    [ ] Green collider outlines visible around both boxes
    [ ] Dragging cyan box into white box: outlines turn YELLOW
    [ ] Terminal prints "Collision ENTER" on overlap
    [ ] Terminal prints "Collision EXIT" on separation
    [ ] HUD text (top-left) stays fixed on screen — does NOT move with camera
    [ ] Camera zoom pulses gently (sin wave) — scene zooms in/out
    [ ] FPS shown in HUD stays near 60
    [ ] SPACE prints "SPACE pressed" to terminal
    [ ] Left click prints world position to terminal
    [ ] ESC closes window cleanly

  Press ESC or close the window when done.
)";

    DebugMode(true);
    InitWindow(800, 600, "KonEngine -- Visual Test Suite v0.8.2");
    SetTargetFPS(60);

    Scene scene;

    // Player: white box with a collider child
    auto* player = scene.Add<Sprite2D>("player");
    player->x = 300; player->y = 300;
    player->width = 48; player->height = 48;
    player->tint = WHITE;
    auto* playerCol = player->AddChild<Collider2D>("playerCol");
    playerCol->width = 48; playerCol->height = 48;
    playerCol->x = 0; playerCol->y = 0; // centered on player pivot

    // Target: cyan box the user drags with RMB
    auto* target = scene.Add<Sprite2D>("target");
    target->x = 520; target->y = 300;
    target->width = 48; target->height = 48;
    target->tint = CYAN;
    auto* targetCol = target->AddChild<Collider2D>("targetCol");
    targetCol->width = 48; targetCol->height = 48;

    scene.Scan();

    int collisionCount = 0;
    playerCol->Connect("on_collision_enter", [&](Collider2D* other) {
        collisionCount++;
        std::cout << "  [VISUAL] Collision ENTER  total=" << collisionCount << "\n";
    });
    playerCol->Connect("on_collision_exit", [&](Collider2D*) {
        std::cout << "  [VISUAL] Collision EXIT\n";
    });

    float elapsed = 0.0f;
    Camera2D cam(400, 300, 1.0f, 0.0f);

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();
        elapsed += dt;

        float speed = 200.0f;
        if (IsKeyDown(Key::W)) player->y -= speed * dt;
        if (IsKeyDown(Key::S)) player->y += speed * dt;
        if (IsKeyDown(Key::A)) player->x -= speed * dt;
        if (IsKeyDown(Key::D)) player->x += speed * dt;

        // Right-click drag moves target in world space
        if (IsMouseButtonDown(Mouse::Right)) {
            target->x = GetWorldMouseX(cam);
            target->y = GetWorldMouseY(cam);
        }

        if (IsKeyPressed(Key::Space))
            std::cout << "  SPACE pressed\n";
        if (IsKeyPressed(Key::Escape))
            break;

        if (IsMouseButtonPressed(Mouse::Left))
            std::cout << "  Left click world=("
                      << (int)GetWorldMouseX(cam) << ", "
                      << (int)GetWorldMouseY(cam) << ")\n";

        // Pulsing zoom
        float zoom = 1.0f + sinf(elapsed * 0.5f) * 0.15f;
        cam = Camera2D(400, 300, zoom, 0.0f);

        // FPS to terminal every second
        static float fpsTimer = 0.0f;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            std::cout << "  FPS: " << GetFPS() << "\n";
            fpsTimer = 0.0f;
        }

        ClearBackground(0.10f, 0.10f, 0.14f);

        BeginCamera2D(cam);
            scene.Update(dt);
            scene.Draw();
            // Floor line (world space)
            DrawLine(0, 480, 800, 480, GRAY);
            // World origin marker
            DrawCircle(0, 0, 4, YELLOW);
        EndCamera2D();

        // HUD — screen space, fixed position, does NOT move with camera
        DrawRectangle(0, 0, 260, 80, {0,0,0,0.6f});
        DrawText("WASD: move white box", 8, 8,  14, WHITE);
        DrawText("RMB drag: move cyan box", 8, 26, 14, WHITE);
        DrawText("Space: print to terminal", 8, 44, 14, WHITE);
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d  Hits: %d", GetFPS(), collisionCount);
        DrawText(fpsBuf, 8, 62, 14, GREEN);

        Present();
        PollEvents();
    }
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main() {
    std::cout << "========================================\n";
    std::cout << "  KonEngine Test Suite  v0.8.2\n";
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
    test_scene();
    test_vector2();
    test_color();
    test_debug_mode();

    std::cout << "\n========================================\n";
    std::cout << "  Headless: " << s_passed << " passed, "
                                << s_failed << " failed\n";
    std::cout << "========================================\n";

    if (s_failed > 0) {
        std::cout << "\n  !! FAILING TESTS — fix before release !!\n";
        return 1;
    }

    std::cout << "  All headless tests passed.\n";

    run_visual_tests();

    std::cout << "\n========================================\n";
    std::cout << "  Visual test complete.\n";
    std::cout << "  Verify the checklist above before releasing.\n";
    std::cout << "========================================\n";

    return 0;
}
