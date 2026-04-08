// little_lad.ks — child node defined in its own file
// This file gets #include'd by main.ks

node LittleLad : Node2D {
    let mut level: I32 = 1;
    let mut experience: I32 = 0;
    let mut hunger: F64 = 100.0;
    let mut sprite: Texture = LoadTexture("liny.png");
    let mut theme: Music = LoadMusic("bgm.mp3");
    let mut themePlaying: Bool = false;

    func Ready() {
        Print("Little lad spawned at level ", level);
    }

    func Feed(amount: F64) {
        hunger = hunger + amount;
        if hunger > 100.0 { hunger = 100.0; }
        Print("Fed! Hunger: ", hunger);
    }

    func GainExp(amount: I32) {
        experience = experience + amount;
        if experience >= level * 100 {
            experience = experience - level * 100;
            level = level + 1;
            Print("Level up! Now level ", level);
        }
    }

    func StartMusic() {
        if !themePlaying {
            PlayMusic(theme);
            themePlaying = true;
        }
    }

    func StopMusic() {
        if themePlaying {
            StopMusicStream(theme);
            themePlaying = false;
        }
    }

    func GetHungerPercent() -> F64 {
        return hunger / 100.0;
    }

    func Update(dt: F64) {
        // Hunger decreases over time
        hunger = hunger - 5.0 * dt;
        if hunger < 0.0 { hunger = 0.0; }

        // Update music stream
        if themePlaying { UpdateMusic(theme); }
    }

    func Draw() {
        // Draw sprite if loaded, otherwise a placeholder
        if sprite.width > 0 {
            DrawTexture(sprite, x - 16.0, y - 16.0, 32.0, 32.0);
        } else {
            DrawRectangle(x - 16.0, y - 16.0, 32.0, 32.0, 0.2, 0.6, 1.0, 1.0);
        }

        // Hunger bar above head
        let barW: F32 = 32.0;
        let barH: F32 = 4.0;
        let barX: F32 = (x - 16.0) as F32;
        let barY: F32 = (y - 24.0) as F32;
        DrawRectangle(barX, barY, barW, barH, 0.3, 0.3, 0.3, 1.0);
        let fillW: F32 = (barW * GetHungerPercent()) as F32;
        if hunger > 50.0 {
            DrawRectangle(barX, barY, fillW, barH, 0.2, 0.8, 0.2, 1.0);
        } else {
            DrawRectangle(barX, barY, fillW, barH, 0.8, 0.2, 0.2, 1.0);
        }
    }

    func OnDestroy() {
        StopMusic();
        UnloadMusic(theme);
        UnloadTexture(sprite);
        Print("Little lad cleaned up");
    }
}
