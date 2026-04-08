#include <engine>

const screenWidth:  I32 = 1000;
const screenHeight: I32 = 600;

node Paddle : Sprite2D {
    const border: F64 = 30;
    let mut col: Collider2D = this.add(Collider2D, "col");

    func Ready() {
        width = 25;
        height = 170;
        col.width = width;
        col.height = height;
    }

    func Update(dt: F64) {
		// making sure the paddle doesn't go past the screen
        if y < (height / 2 + border) { y = height / 2 + border; }
        if y > (screenHeight - height / 2 - border) { y = screenHeight - height / 2 - border; }
    }
}

node Ball : Sprite2D {
    const speed: F64 = 200;
    let mut dir: Vec2 = Vec2(1.0, 1.0);
    let mut col: Collider2D = this.add(Collider2D, "col");

    func Ready() {
        x = 500;
        y = 300;
        width = 35;
        height = 35;
        col.width = width;
        col.height = height;
    }

    func Update(dt: F64) {
		// makes it change direciton when hitting the top or bottom of the screen
        if y - height / 2 < 0 { dir.y =  1; }
        else if y + height / 2 > screenHeight { dir.y = -1; }

		// 
        x += dir.x * dt * speed;
        y += dir.y * dt * speed;
    }

    func OnCollisionEnter(other: Collider2D) {
        dir.x = -dir.x;
    }
}

func main() -> I32 {
    InitWindow(1000, 600, "Pong");
    SetTargetFPS(60);
    DebugMode(true);

    let mut scene: Scene = Scene();
    let mut ball: Ball = scene.add(Ball, "ball");

    let mut player: Paddle = scene.add(Paddle, "player");
    player.x = 50;
    player.y = screenHeight / 2;

    let mut enemy: Paddle = scene.add(Paddle, "enemy");
    enemy.x = 950;
    enemy.y = screenHeight / 2;

    while !WindowShouldClose() {
        let mut dt: F64 = GetDeltaTime();
        player.y = GetMouseY();
        enemy.y = ball.y;
        scene.update(dt);

        ClearBackground(0.0, 0.0, 0.0);
        scene.draw();
        Present();
        PollEvents();
    }
    return 0;
}
