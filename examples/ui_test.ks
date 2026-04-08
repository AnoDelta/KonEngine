#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// UI Test — buttons, labels, panels, signals
//
// Tests: Button creation, click handlers, hover signals,
// labels, panels with children, input blocking
//
// Controls:
//   Mouse        = interact with UI
//   F1           = toggle debug mode
// ═══════════════════════════════════════════════════════════════════════

func main() {
    InitWindow(800, 600, "UI Test", false);
    SetTargetFPS(60);

    let mut clickCount: I32 = 0;
    let mut message: Str = "Click a button!";
    let mut bgR: F32 = 0.1;
    let mut bgG: F32 = 0.1;
    let mut bgB: F32 = 0.15;

    // ── Create UI elements ──

    // Title label
    UI.AddLabel("title", "KonEngine UI Test", 300.0, 20.0, 24, WHITE);

    // Buttons
    UI.AddButton("click_me", "Click Me!", 50.0, 100.0);
    UI.AddButton("reset", "Reset", 50.0, 160.0);

    // Color buttons
    UI.AddLabel("color_label", "Background:", 50.0, 240.0);
    UI.AddButton("red", "Red", 50.0, 270.0);
    UI.AddButton("green", "Green", 170.0, 270.0);
    UI.AddButton("blue", "Blue", 290.0, 270.0);
    UI.AddButton("dark", "Dark", 410.0, 270.0);

    // Status label (updated dynamically)
    UI.AddLabel("status", "Clicks: 0", 50.0, 350.0);
    UI.AddLabel("message", "Click a button!", 50.0, 380.0);

    // Panel with child buttons
    UI.AddPanel("panel", 500.0, 100.0, 250.0, 180.0);
    UI.AddButton("panel_btn1", "Panel Button 1", 20.0, 30.0);
    UI.AddButton("panel_btn2", "Panel Button 2", 20.0, 90.0);
    UI.PanelAddChild("panel", "panel_btn1");
    UI.PanelAddChild("panel", "panel_btn2");
    UI.AddLabel("panel_title", "Panel Demo", 520.0, 105.0, 16, YELLOW);

    // ── Connect signals ──

    UI.OnClick("click_me", [&]() {
        clickCount = clickCount + 1;
        message = "Clicked! Total: " + ToString(clickCount);
    });

    UI.OnClick("reset", [&]() {
        clickCount = 0;
        message = "Counter reset!";
    });

    UI.OnClick("red", [&]() {
        bgR = 0.3; bgG = 0.08; bgB = 0.08;
        message = "Background: Red";
    });

    UI.OnClick("green", [&]() {
        bgR = 0.08; bgG = 0.25; bgB = 0.08;
        message = "Background: Green";
    });

    UI.OnClick("blue", [&]() {
        bgR = 0.08; bgG = 0.08; bgB = 0.3;
        message = "Background: Blue";
    });

    UI.OnClick("dark", [&]() {
        bgR = 0.1; bgG = 0.1; bgB = 0.15;
        message = "Background: Dark";
    });

    UI.OnClick("panel_btn1", [&]() {
        message = "Panel Button 1 clicked!";
    });

    UI.OnClick("panel_btn2", [&]() {
        message = "Panel Button 2 clicked!";
    });

    // Hover signals
    UI.Connect("click_me", "hovered", [&]() {
        Print("Mouse entered Click Me button");
    });

    while !WindowShouldClose() {
        // Toggle debug
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }

        // Update UI (hit testing, signals)
        UI.Update();

        // Only handle world clicks if UI didn't consume input
        if !UI.WantsInput() && MousePressed(Mouse.Left) {
            message = "Clicked on background";
        }

        // ── Render ──
        ClearBackground(bgR, bgG, bgB);

        // Draw some world content behind UI
        DrawRectangle(300.0, 450.0, 200.0, 100.0, 0.2, 0.2, 0.3, 1.0);
        DrawText("World content (behind UI)", 310.0, 480.0, 14, GRAY);

        // Update dynamic labels
        UI.Remove("status");
        UI.AddLabel("status", "Clicks: " + ToString(clickCount), 50.0, 350.0);
        UI.Remove("message");
        UI.AddLabel("message", message, 50.0, 380.0);

        // Draw all UI elements
        UI.Draw();

        Present();
        PollEvents();
    }
}
