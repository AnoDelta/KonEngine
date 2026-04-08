// main.ks — multi-file example
// Demonstrates: #include for separate node files, custom methods,
// Texture/Music fields on nodes, custom functions

#include <engine>
#include "little_lad.ks"

node GameManager : Node2D {
    let mut elapsedTime: F64 = 0.0;
    let mut feedTimer: F64 = 0.0;

    func Ready() {
        Print("Game Manager initialized");
    }

    func Update(dt: F64) {
        elapsedTime = elapsedTime + dt;
        feedTimer = feedTimer + dt;
    }

    func GetElapsed() -> F64 {
        return elapsedTime;
    }
}

func main() -> I32 {
    InitWindow(800, 600, "Multi-File Demo", false);
    SetTargetFPS(60);

    let mut scene: Scene = Scene();
    let mut manager: GameManager = scene.add(GameManager, "manager");
    let mut lad: LittleLad = scene.add(LittleLad, "lad");
    lad.x = 400.0;
    lad.y = 300.0;

    // Start music from the child node
    lad.StartMusic();

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        scene.update(dt);

        // Feed the lad every 3 seconds
        if manager.feedTimer > 3.0 {
            manager.feedTimer = 0.0;
            lad.Feed(20.0);
            lad.GainExp(25);
        }

        // Manual feeding with spacebar
        if KeyPressed(Key.Space) {
            lad.Feed(30.0);
        }

        // Toggle music
        if KeyPressed(Key.M) {
            if lad.themePlaying {
                lad.StopMusic();
            } else {
                lad.StartMusic();
            }
        }

        ClearBackground(0.08, 0.08, 0.12);
        scene.draw();

        // HUD
        DrawText("Multi-File Node Demo", 10.0, 10.0, 20, WHITE);
        DrawText("Space: feed | M: toggle music", 10.0, 35.0, 14, GRAY);
        DrawText("Level: " + ToString(lad.level), 10.0, 60.0, 18, YELLOW);
        DrawText("Exp: " + ToString(lad.experience), 10.0, 82.0, 16, WHITE);
        DrawText("Hunger: " + ToString(lad.hunger), 10.0, 104.0, 16, WHITE);
        DrawText("Time: " + ToString(manager.GetElapsed()), 10.0, 126.0, 14, GRAY);

        Present();
        PollEvents();
    }

    return 0;
}
