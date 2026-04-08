#include <engine>

// Simple platformer with physics collision response
// Player can't pass through walls/floors

node Player : KinematicBody2D {
    let mut col: Collider2D = this.add(Collider2D, "col");
    let mut speed: F64 = 200;
    let mut jumpForce: F64 = -400;
    let mut vy: F64 = 0;
    let mut gravity: F64 = 980;
    let mut onGround: Bool = false;

    func Ready() {
        x = 100;
        y = 400;
        col.width = 32;
        col.height = 48;
    }

    func Update(dt: F64) {
        // Horizontal movement
        let mut dx: F64 = 0;
        if IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)  { dx = -speed * dt; }
        if IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT) { dx =  speed * dt; }

        // Gravity
        vy = vy + gravity * dt;
        let mut dy: F64 = vy * dt;

        // Jump
        if onGround && IsKeyPressed(KEY_SPACE) {
            vy = jumpForce;
            onGround = false;
        }

        // Move with collision — prevents passing through walls
        MoveAndCollide(dx, dy);
    }

    func OnCollisionEnter(other: Collider2D) {
        // If we hit something below us, we're on the ground
        if vy > 0 { onGround = true; vy = 0; }
    }
}

node Wall : StaticBody2D {
    func Ready() {
        AddCollider(800, 32);
    }
}

func main() -> I32 {
    InitWindow(800, 600, "Platformer");
    SetTargetFPS(60);

    let mut scene: Scene = Scene();

    // Player
    let mut player: Player = scene.add(Player, "player");

    // Floor
    let mut floor: Wall = scene.add(Wall, "floor");
    floor.x = 400;
    floor.y = 568;

    // Platforms
    let mut plat1: Wall = scene.add(Wall, "plat1");
    plat1.x = 300;
    plat1.y = 450;

    let mut plat2: Wall = scene.add(Wall, "plat2");
    plat2.x = 550;
    plat2.y = 350;

    while !WindowShouldClose() {
        scene.update(GetDeltaTime());
        ClearBackground(0.05, 0.05, 0.1);
        scene.draw();
        Present();
        PollEvents();
    }
    return 0;
}
