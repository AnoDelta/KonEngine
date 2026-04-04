#include <engine>

// -----------------------------------------------------------------------
// KonScript test game — tests the full KonScript + engine integration.
// This covers things the C++ test suite can't: codegen correctness,
// KonScript node lifecycle, collision bubbling, animation, scene scan.
//
// Run with:  ksc test_game.ks
// Or build into a project and run the executable.
//
// Expected output in terminal (with DebugMode on):
//   [TestGame] All nodes Ready
//   [Collision] Player hit wall (enter)
//   [Collision] Player left wall (exit)
//   [Anim] Switched to walk
//   [Anim] Switched to idle
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Test: OnCollisionEnter bubbles to parent node
// -----------------------------------------------------------------------
node Player : Sprite2D {
    let mut velX:      F64  = 0.0;
    let mut velY:      F64  = 0.0;
    let mut grounded:  Bool = false;
    let mut speed:     F64  = 220.0;
    let mut gravity:   F64  = 900.0;
    let mut hits:      I32  = 0;
    let mut anim:      AnimationPlayer = this.add(AnimationPlayer, "anim");

    func Ready() {
        x       = 120.0;
        y       = 400.0;
        originX = 0.5;
        originY = 1.0;
        scaleX  = 2.0;
        scaleY  = 2.0;

        let col: Collider2D = this.add(Collider2D, "playerCol");
        col.width  = 28.0;
        col.height = 48.0;
        col.x      = 0.0;
        col.y      = -24.0;

        anim.LoadFromFile("player.konani");
        anim.Play("idle");

        Print("[TestGame] Player Ready\n");
    }

    func Update(dt: F64) {
        velX = 0.0;

        if KeyDown(Key.D) {
            velX   = speed;
            scaleX = 2.0;
        }
        if KeyDown(Key.A) {
            velX   = -speed;
            scaleX = -2.0;
        }
        x += velX * dt;

        if KeyPressed(Key.Space) && grounded {
            velY     = -500.0;
            grounded = false;
        }

        velY += gravity * dt;
        y    += velY * dt;

        if y >= 500.0 {
            y        = 500.0;
            velY     = 0.0;
            grounded = true;
        }

        if x < 0.0   { x = 0.0; }
        if x > 800.0 { x = 800.0; }

        // Animation state machine
        if grounded {
            if velX != 0.0 {
                anim.Play("walk");
            } else {
                anim.Play("idle");
            }
        }

        DrawText("WASD:move  Space:jump", 10, 10, 14, WHITE);
        DrawText("Collisions: %d", 10, 28, 14, YELLOW, hits);
        DrawText("Press ESC to quit", 10, 46, 14, GRAY);
    }

    func OnCollisionEnter(other: Collider2D) {
        hits += 1;
        Print("[Collision] Player hit %s  total=%d\n", other.name, hits);
    }

    func OnCollisionExit(other: Collider2D) {
        Print("[Collision] Player left %s\n", other.name);
    }
}

// -----------------------------------------------------------------------
// Test: Node field collider with correct default size
// -----------------------------------------------------------------------
node Wall : Node2D {
    let mut w: F64 = 80.0;
    let mut h: F64 = 300.0;
    let mut col: Collider2D = this.add(Collider2D, "wallCol");

    func Ready() {
        col.width  = w;
        col.height = h;
        Print("[TestGame] Wall Ready  col=%fx%f\n", col.width, col.height);
    }

    func Draw() {
        DrawRectangle(x - w * 0.5, y - h * 0.5, w, h, ORANGE);
        DrawRectangle(x - w * 0.5, y - h * 0.5, w, 3.0, RED);
    }
}

// -----------------------------------------------------------------------
// Test: Spawning dynamic boxes and scanning
// -----------------------------------------------------------------------
node Box : Node2D {
    let mut size: F64 = 40.0;
    let mut col:  Collider2D = this.add(Collider2D, "boxCol");

    func Ready() {
        col.width  = size;
        col.height = size;
    }

    func Draw() {
        DrawRectangle(x - size * 0.5, y - size * 0.5, size, size, CYAN);
    }
}

func main() {
    InitWindow(800, 600, "KonEngine -- KonScript Test Game");
    SetTargetFPS(60);
    DebugMode(true);

    let mut tex: Texture = LoadTexture("player.png");

    let scene: Scene = Scene();

    let player: Player = scene.add(Player, "player");
    player.SetTexture(tex);

    let wall: Wall = scene.add(Wall, "wall");
    wall.x = 600.0;
    wall.y = 350.0;
    scene.scan();

    let mut cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);
    let mut boxCount: I32 = 0;

    Print("[TestGame] All nodes Ready\n");

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();

        if KeyPressed(Key.Escape) { break; }

        // Spawn box on left click
        if MousePressed(Mouse.Left) {
            boxCount += 1;
            let box: Box = scene.add(Box, "box");
            box.x = GetWorldMouseX(cam);
            box.y = GetWorldMouseY(cam);
            scene.scan();
        }

        // Remove all boxes on right click
        if MousePressed(Mouse.Right) {
            for i: I32 in 0..boxCount {
                scene.remove("box");
            }
            boxCount = 0;
            scene.scan();
        }

        let scroll: F64 = GetMouseScroll();
        if scroll != 0.0 {
            let z: F64 = cam.zoom + scroll * 0.1;
            if z < 0.1 { cam = Camera2D(cam.x, cam.y, 0.1, cam.rotation); }
            if z > 5.0 { cam = Camera2D(cam.x, cam.y, 5.0, cam.rotation); }
        }

        ClearBackground(0.08, 0.08, 0.12);

        BeginCamera2D(cam);
            scene.update(dt);
            scene.draw();
            DrawLine(0.0, 500.0, 800.0, 500.0, GRAY);
        EndCamera2D();

        DrawText("FPS: %d", 10, 580, 12, GREEN, GetFPS());
        DrawText("Boxes: %d  (LClick=add  RClick=clear)", 10, 565, 12, GRAY, boxCount);

        Present();
        PollEvents();
    }
}
