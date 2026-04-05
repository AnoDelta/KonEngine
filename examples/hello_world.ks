#include <engine>

node Logo : Sprite2D {
    func Ready() {
        x = 400;
        y = 300;
        width = 200;
        height = 200;
    }
}

func main() -> I32 {
    InitWindow(800, 600, "Hello KonEngine");
    SetTargetFPS(60);

    let mut tex: Texture = LoadTexture("logo.png");
    let mut scene: Scene = Scene();
    let mut logo: Logo = scene.add(Logo, "logo");
    logo.SetTexture(tex);

    while !WindowShouldClose() {
        scene.update(GetDeltaTime());
        ClearBackground(0.1, 0.1, 0.15);
        scene.draw();
        Present();
        PollEvents();
    }

    UnloadTexture(tex);
    return 0;
}
