#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Physics Test — monolithic file to test all physics features
//
// Tests: RigidBody2D gravity, KinematicBody2D movement, StaticBody2D
// walls, collision enter/exit signals, floor detection, jumping,
// multiple colliders, layer/mask filtering
//
// Controls:
//   WASD / Arrows = move player
//   Space         = jump
//   R             = reset player position
//   D             = toggle debug mode
// ═══════════════════════════════════════════════════════════════════════

const PLAYER_SPEED: F64 = 250.0;
const JUMP_FORCE:   F64 = -450.0;
const GRAVITY:      F64 = 980.0;

// ── Player (KinematicBody2D) ─────────────────────────────────────────
// Manual gravity + MoveAndCollide for precise control
node Player : KinematicBody2D {
    let mut vy: F64 = 0.0;
    let mut onGround: Bool = false;
    let mut jumpCount: I32 = 0;
    let mut touchingWall: Bool = false;

    func Ready() {
        x = 100.0;
        y = 400.0;
        let col: Collider2D = this.add(Collider2D, "body");
        col.width = 28.0;
        col.height = 44.0;
    }

    func Update(dt: F64) {
        let mut dx: F64 = 0.0;
        if KeyDown(Key.D) || KeyDown(Key.Right) { dx = PLAYER_SPEED * dt; }
        if KeyDown(Key.A) || KeyDown(Key.Left)  { dx = -PLAYER_SPEED * dt; }

        // Gravity
        vy = vy + GRAVITY * dt;
        let mut dy: F64 = vy * dt;

        // Jump (double jump allowed)
        if KeyPressed(Key.Space) && jumpCount < 2 {
            vy = JUMP_FORCE;
            onGround = false;
            jumpCount = jumpCount + 1;
        }

        // Reset
        if KeyPressed(Key.R) {
            x = 100.0; y = 400.0;
            vy = 0.0; onGround = false; jumpCount = 0;
        }

        MoveAndCollide(dx, dy);
    }

    func OnCollisionEnter(other: Collider2D) {
        if vy > 0.0 {
            onGround = true;
            vy = 0.0;
            jumpCount = 0;
        }
        if other.name == "wall_col" {
            touchingWall = true;
        }
    }

    func OnCollisionExit(other: Collider2D) {
        if other.name == "wall_col" {
            touchingWall = false;
        }
    }

    func Draw() {
        let c: Color = touchingWall ? Color(1.0, 0.3, 0.3, 1.0) : Color(0.2, 0.6, 1.0, 1.0);
        DrawRectangle(x - 14.0, y - 22.0, 28.0, 44.0, c);
    }
}

// ── Crate (RigidBody2D) ─────────────────────────────────────────────
// Automatic gravity + physics. Falls and rests on platforms.
node Crate : RigidBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "crate_col");
        col.width = 24.0;
        col.height = 24.0;
        gravity = GRAVITY;
    }

    func Draw() {
        DrawRectangle(x - 12.0, y - 12.0, 24.0, 24.0, Color(0.8, 0.6, 0.2, 1.0));
    }
}

// ── Wall (StaticBody2D) ─────────────────────────────────────────────
// Immovable. Used for floor, walls, platforms.
node Wall : StaticBody2D {
    let mut w: F64 = 100.0;
    let mut h: F64 = 20.0;

    func Ready() {
        let col: Collider2D = this.add(Collider2D, "wall_col");
        col.width = w;
        col.height = h;
    }

    func Draw() {
        DrawRectangle(x, y, w, h, Color(0.3, 0.3, 0.35, 1.0));
    }
}

// ── Main ─────────────────────────────────────────────────────────────
func main() {
    InitWindow(800, 600, "Physics Test");
    SetTargetFPS(60);
    Random.Seed();

    let scene: Scene = Scene();

    // Player
    let player: Player = scene.add(Player, "player");

    // Floor (full width)
    let floor: Wall = scene.add(Wall, "floor");
    floor.w = 800.0; floor.h = 32.0;
    floor.x = 0.0; floor.y = 568.0;

    // Left wall
    let leftWall: Wall = scene.add(Wall, "leftWall");
    leftWall.w = 20.0; leftWall.h = 600.0;
    leftWall.x = 0.0; leftWall.y = 0.0;

    // Right wall
    let rightWall: Wall = scene.add(Wall, "rightWall");
    rightWall.w = 20.0; rightWall.h = 600.0;
    rightWall.x = 780.0; rightWall.y = 0.0;

    // Platforms at different heights
    let plat1: Wall = scene.add(Wall, "plat1");
    plat1.w = 150.0; plat1.h = 16.0;
    plat1.x = 200.0; plat1.y = 470.0;

    let plat2: Wall = scene.add(Wall, "plat2");
    plat2.w = 120.0; plat2.h = 16.0;
    plat2.x = 450.0; plat2.y = 380.0;

    let plat3: Wall = scene.add(Wall, "plat3");
    plat3.w = 180.0; plat3.h = 16.0;
    plat3.x = 300.0; plat3.y = 280.0;

    let plat4: Wall = scene.add(Wall, "plat4");
    plat4.w = 100.0; plat4.h = 16.0;
    plat4.x = 600.0; plat4.y = 200.0;

    // Crates that fall with gravity
    let crate1: Crate = scene.add(Crate, "crate1");
    crate1.x = 250.0; crate1.y = 100.0;

    let crate2: Crate = scene.add(Crate, "crate2");
    crate2.x = 280.0; crate2.y = 50.0;

    let crate3: Crate = scene.add(Crate, "crate3");
    crate3.x = 500.0; crate3.y = 150.0;

    // Re-register all colliders after setup
    scene.scan();

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();

        // Toggle debug
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }

        ClearBackground(0.08, 0.08, 0.12);
        scene.update(dt);
        scene.draw();

        // HUD
        DrawText("Physics Test", 10.0, 10.0, 20, WHITE);
        DrawText("WASD/Arrows: move | Space: jump (double!) | R: reset | F1: debug", 10.0, 35.0, 14, GRAY);

        let groundText: Str = player.onGround ? "On Ground" : "In Air";
        let groundColor: Color = player.onGround ? GREEN : YELLOW;
        DrawText(groundText, 10.0, 55.0, 14, groundColor);

        if player.touchingWall {
            DrawText("Touching Wall", 10.0, 72.0, 14, RED);
        }

        Present();
        PollEvents();
    }
}
