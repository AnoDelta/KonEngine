#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Physics Test
//
// Controls: WASD/Arrows = move, Space = jump, R = reset, F1 = debug
// ═══════════════════════════════════════════════════════════════════════

const SPEED: F64 = 250.0;
const JUMP:  F64 = -400.0;
const GRAV:  F64 = 800.0;

node Player : Node2D {
    let mut vy: F64 = 0.0;
    let mut onGround: Bool = false;

    func Ready() {
        x = 400.0;
        y = 300.0;
    }

    func Update(dt: F64) {
        // Horizontal
        if KeyDown(Key.D) || KeyDown(Key.Right) { x += SPEED * dt; }
        if KeyDown(Key.A) || KeyDown(Key.Left)  { x -= SPEED * dt; }

        // Gravity
        vy = vy + GRAV * dt;

        // Jump
        if onGround && KeyPressed(Key.Space) {
            vy = JUMP;
            onGround = false;
        }

        // Apply vertical movement
        y = y + vy * dt;

        // Reset
        if KeyPressed(Key.R) { x = 400.0; y = 300.0; vy = 0.0; }

        // ── Manual collision with floor ──
        // Floor is at y=568, player is 44px tall, centered (so bottom = y + 22)
        let bottom: F64 = y + 22.0;
        if bottom > 568.0 {
            y = 568.0 - 22.0;
            vy = 0.0;
            onGround = true;
        }

        // Ceiling at y=0
        let top: F64 = y - 22.0;
        if top < 0.0 {
            y = 22.0;
            vy = 0.0;
        }

        // Left wall at x=20
        if x - 14.0 < 20.0 { x = 20.0 + 14.0; }

        // Right wall at x=780
        if x + 14.0 > 780.0 { x = 780.0 - 14.0; }

        // Platform 1: (200, 470) 150x16
        let px1: F64 = 200.0;
        let py1: F64 = 470.0;
        if x + 14.0 > px1 && x - 14.0 < px1 + 150.0 {
            if bottom > py1 && bottom < py1 + 16.0 && vy > 0.0 {
                y = py1 - 22.0;
                vy = 0.0;
                onGround = true;
            }
        }

        // Platform 2: (450, 380) 120x16
        let px2: F64 = 450.0;
        let py2: F64 = 380.0;
        if x + 14.0 > px2 && x - 14.0 < px2 + 120.0 {
            if bottom > py2 && bottom < py2 + 16.0 && vy > 0.0 {
                y = py2 - 22.0;
                vy = 0.0;
                onGround = true;
            }
        }

        // Platform 3: (300, 280) 180x16
        let px3: F64 = 300.0;
        let py3: F64 = 280.0;
        if x + 14.0 > px3 && x - 14.0 < px3 + 180.0 {
            if bottom > py3 && bottom < py3 + 16.0 && vy > 0.0 {
                y = py3 - 22.0;
                vy = 0.0;
                onGround = true;
            }
        }
    }

    func Draw() {
        DrawRectangle(x - 14.0, y - 22.0, 28.0, 44.0, Color(0.2, 0.6, 1.0, 1.0));
    }
}

func main() {
    InitWindow(800, 600, "Physics Test");
    SetTargetFPS(60);

    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }

        ClearBackground(0.08, 0.08, 0.12);
        scene.update(dt);

        // Draw walls manually
        DrawRectangle(0.0, 568.0, 800.0, 32.0, Color(0.3, 0.3, 0.35, 1.0));
        DrawRectangle(0.0, 0.0, 20.0, 600.0, Color(0.3, 0.3, 0.35, 1.0));
        DrawRectangle(780.0, 0.0, 20.0, 600.0, Color(0.3, 0.3, 0.35, 1.0));

        // Draw platforms
        DrawRectangle(200.0, 470.0, 150.0, 16.0, Color(0.25, 0.35, 0.3, 1.0));
        DrawRectangle(450.0, 380.0, 120.0, 16.0, Color(0.25, 0.35, 0.3, 1.0));
        DrawRectangle(300.0, 280.0, 180.0, 16.0, Color(0.25, 0.35, 0.3, 1.0));

        scene.draw();

        // HUD
        DrawText("Physics Test", 10.0, 10.0, 20, WHITE);
        DrawText("WASD: move | Space: jump | R: reset | F1: debug", 10.0, 35.0, 14, GRAY);
        if player.onGround {
            DrawText("On Ground", 10.0, 55.0, 14, GREEN);
        } else {
            DrawText("In Air", 10.0, 55.0, 14, YELLOW);
        }

        Present();
        PollEvents();
    }
}
