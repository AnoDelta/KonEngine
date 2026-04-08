#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Timer Test — frame-rate independent timers
//
// Tests: repeating timers, one-shot timers, pause/resume,
// remaining time queries, timer removal
//
// Controls:
//   Space        = pause/resume counter timer
//   R            = reset counter timer
//   X            = remove counter timer
//   C            = create counter timer (if removed)
//   F1           = toggle debug mode
// ═══════════════════════════════════════════════════════════════════════

func main() {
    InitWindow(800, 600, "Timer Test", false);
    SetTargetFPS(60);

    let mut score: I32 = 0;
    let mut blink: Bool = true;
    let mut alertFired: Bool = false;
    let mut paused: Bool = false;
    let mut timerExists: Bool = true;
    let mut spawnCount: I32 = 0;

    // ── Create timers ──

    // Repeating: adds 10 points every second
    Timer.Create("counter", 1.0, true, [&]() {
        score = score + 10;
        Print("Score: ", score);
    });

    // Repeating: blink effect every 0.5s
    Timer.Create("blink", 0.5, true, [&]() {
        blink = !blink;
    });

    // One-shot: fires after 5 seconds
    Timer.Create("alert", 5.0, false, [&]() {
        alertFired = true;
        Print("Alert: 5 seconds have passed!");
    });

    // Fast repeating: spawn counter every 0.25s
    Timer.Create("spawn", 0.25, true, [&]() {
        spawnCount = spawnCount + 1;
    });

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();

        // Tick all timers
        Timer.UpdateAll(dt);

        // Toggle debug
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }

        // Pause / resume the counter timer
        if KeyPressed(Key.Space) {
            if paused {
                Timer.Resume("counter");
                paused = false;
            } else {
                Timer.Pause("counter");
                paused = true;
            }
        }

        // Reset counter timer
        if KeyPressed(Key.R) {
            Timer.Reset("counter");
            score = 0;
        }

        // Remove counter timer
        if KeyPressed(Key.X) && timerExists {
            Timer.Remove("counter");
            timerExists = false;
        }

        // Recreate counter timer
        if KeyPressed(Key.C) && !timerExists {
            Timer.Create("counter", 1.0, true, [&]() {
                score = score + 10;
            });
            timerExists = true;
            paused = false;
        }

        // ── Render ──
        ClearBackground(0.08, 0.08, 0.12);

        // Title
        DrawText("Timer Test", 10.0, 10.0, 24, WHITE);
        DrawText("Space: pause/resume | R: reset | X: remove | C: create", 10.0, 40.0, 14, GRAY);

        // Score display (blinks)
        if blink {
            DrawText("Score: " + ToString(score), 10.0, 80.0, 28, YELLOW);
        } else {
            DrawText("Score: " + ToString(score), 10.0, 80.0, 28, ORANGE);
        }

        // Counter timer status
        if timerExists {
            if paused {
                DrawText("Counter: PAUSED", 10.0, 120.0, 18, RED);
            } else {
                let rem: F64 = Timer.Remaining("counter");
                DrawRectangle(10.0, 120.0, 200.0, 20.0, 0.2, 0.2, 0.2, 1.0);
                // Progress bar: fills up as timer counts down
                let progress: F64 = 1.0 - rem;
                if progress < 0.0 { progress = 0.0; }
                if progress > 1.0 { progress = 1.0; }
                DrawRectangle(10.0, 120.0, (200.0 * progress) as F32, 20.0, 0.2, 0.7, 0.3, 1.0);
                DrawText("Counter: " + ToString(rem) + "s", 220.0, 122.0, 16, GREEN);
            }
        } else {
            DrawText("Counter: REMOVED (press C to create)", 10.0, 120.0, 18, GRAY);
        }

        // Alert timer
        if alertFired {
            DrawText("Alert: FIRED!", 10.0, 160.0, 18, CYAN);
        } else {
            let alertRem: F64 = Timer.Remaining("alert");
            DrawText("Alert fires in: " + ToString(alertRem) + "s", 10.0, 160.0, 18, WHITE);
        }

        // Spawn counter
        DrawText("Spawns (0.25s timer): " + ToString(spawnCount), 10.0, 200.0, 16, MAGENTA);

        // Timer existence queries
        DrawText("Timer.Exists(\"counter\"): " + ToString(Timer.Exists("counter")), 10.0, 240.0, 14, GRAY);
        DrawText("Timer.Exists(\"alert\"): " + ToString(Timer.Exists("alert")), 10.0, 260.0, 14, GRAY);
        DrawText("Timer.Finished(\"alert\"): " + ToString(Timer.Finished("alert")), 10.0, 280.0, 14, GRAY);

        // Visual: draw spawn dots
        let mut si: I32 = 0;
        while si < spawnCount && si < 100 {
            let sx: F32 = (400 + (si % 20) * 18) as F32;
            let sy: F32 = (80 + (si / 20) * 18) as F32;
            DrawRectangle(sx, sy, 14.0, 14.0, 0.8, 0.3, 0.8, 1.0);
            si = si + 1;
        }
        if spawnCount > 0 {
            DrawText("Spawned objects", 400.0, 60.0, 14, MAGENTA);
        }

        Present();
        PollEvents();
    }
}
