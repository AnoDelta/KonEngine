#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Physics Test — uses engine collision system
//
// Controls: WASD/Arrows = move, Space = jump, R = reset, F1 = debug
//           F2 = collision log on, F3 = collision log off
// ═══════════════════════════════════════════════════════════════════════

const SPEED: F64 = 250.0;
const JUMP:  F64 = -400.0;
const GRAV:  F64 = 800.0;

// Everything uses origin 0.5 (centered) — position is the CENTER of the object.
// Draw functions also draw centered: DrawRectangle(x - w/2, y - h/2, w, h, color)

node Player : KinematicBody2D {
    let mut vy: F64 = 0.0;
    let mut onGround: Bool = false;

    func Ready() {
        x = 400.0;
        y = 300.0;
        let col: Collider2D = this.add(Collider2D, "body");
        col.width = 28.0;
        col.height = 44.0;
    }

    func Update(dt: F64) {
        if KeyDown(Key.D) || KeyDown(Key.Right) { x += SPEED * dt; }
        if KeyDown(Key.A) || KeyDown(Key.Left)  { x -= SPEED * dt; }

        vy = vy + GRAV * dt;
        let dy: F64 = vy * dt;

        if onGround && KeyPressed(Key.Space) {
            vy = JUMP;
            onGround = false;
        }

        if KeyPressed(Key.R) { x = 400.0; y = 300.0; vy = 0.0; }

        let beforeY: F64 = y;
        MoveAndCollide(0.0, dy);
        let afterY: F64 = y;
        let actualDy: F64 = afterY - beforeY;

        // Floor detection: if we moved less than we wanted (got pushed back)
        if dy > 0.001 && actualDy < dy - 0.001 {
            onGround = true;
            vy = 0.0;
        } else if dy > 0.001 {
            onGround = false;
        }
    }

    func Draw() {
        DrawRectangle(x - 14.0, y - 22.0, 28.0, 44.0, Color(0.2, 0.6, 1.0, 1.0));
    }
}

// All walls/platforms: origin 0.5 (default), position = center.
// Draw centered to match collider.

node Floor : StaticBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 800.0; col.height = 32.0;
    }
    func Draw() { DrawRectangle(x - 400.0, y - 16.0, 800.0, 32.0, Color(0.3, 0.3, 0.35, 1.0)); }
}

node SideWall : StaticBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 20.0; col.height = 600.0;
    }
    func Draw() { DrawRectangle(x - 10.0, y - 300.0, 20.0, 600.0, Color(0.3, 0.3, 0.35, 1.0)); }
}

node Plat150 : StaticBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 150.0; col.height = 16.0;
    }
    func Draw() { DrawRectangle(x - 75.0, y - 8.0, 150.0, 16.0, Color(0.25, 0.35, 0.3, 1.0)); }
}

node Plat120 : StaticBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 120.0; col.height = 16.0;
    }
    func Draw() { DrawRectangle(x - 60.0, y - 8.0, 120.0, 16.0, Color(0.25, 0.35, 0.3, 1.0)); }
}

node Plat180 : StaticBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 180.0; col.height = 16.0;
    }
    func Draw() { DrawRectangle(x - 90.0, y - 8.0, 180.0, 16.0, Color(0.25, 0.35, 0.3, 1.0)); }
}

func main() {
    InitWindow(800, 600, "Physics Test");
    SetTargetFPS(60);

    let scene: Scene = Scene();
    CollisionDebug(true);
    let player: Player = scene.add(Player, "player");

    // Positions are CENTER of each wall
    let floor: Floor = scene.add(Floor, "floor");
    floor.x = 400.0; floor.y = 584.0;

    let lw: SideWall = scene.add(SideWall, "lw");
    lw.x = 10.0; lw.y = 300.0;

    let rw: SideWall = scene.add(SideWall, "rw");
    rw.x = 790.0; rw.y = 300.0;

    let p1: Plat150 = scene.add(Plat150, "p1");
    p1.x = 275.0; p1.y = 478.0;

    let p2: Plat120 = scene.add(Plat120, "p2");
    p2.x = 510.0; p2.y = 388.0;

    let p3: Plat180 = scene.add(Plat180, "p3");
    p3.x = 390.0; p3.y = 288.0;

    scene.scan();

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }
        if KeyPressed(Key.F2) { CollisionDebug(true); }
        if KeyPressed(Key.F3) { CollisionDebug(false); }

        ClearBackground(0.08, 0.08, 0.12);
        scene.update(dt);
        scene.draw();

        DrawText("Physics Test (Engine Collision)", 10.0, 10.0, 20, WHITE);
        DrawText("WASD: move | Space: jump | R: reset | F1: debug | F2/F3: collision log", 10.0, 35.0, 14, GRAY);
        if player.onGround {
            DrawText("On Ground", 10.0, 55.0, 14, GREEN);
        } else {
            DrawText("In Air", 10.0, 55.0, 14, YELLOW);
        }

        Present();
        PollEvents();
    }
}
