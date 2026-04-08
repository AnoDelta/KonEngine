#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Tilemap Test — monolithic file to test tilemap and grid features
//
// Tests: Tilemap data storage, tile placement/removal, grid drawing,
// coordinate conversion, click detection, camera with tilemap
//
// Controls:
//   WASD         = pan camera
//   Scroll       = zoom
//   Left click   = place tile (cycles through IDs)
//   Right click  = remove tile
//   1-5          = select tile ID
//   G            = toggle grid overlay
//   F1           = toggle debug mode
// ═══════════════════════════════════════════════════════════════════════

const MAP_COLS: I32 = 25;
const MAP_ROWS: I32 = 19;
const TILE_W: I32 = 32;
const TILE_H: I32 = 32;
const CAM_SPEED: F64 = 300.0;

func main() {
    InitWindow(800, 600, "Tilemap Test", false);
    SetTargetFPS(60);

    // Create tilemap
    let mut map: Tilemap = Tilemap(MAP_COLS, MAP_ROWS, TILE_W, TILE_H);
    map.originX = 0.0;
    map.originY = 0.0;

    // Fill a checkerboard pattern
    let mut y: I32 = 0;
    while y < MAP_ROWS {
        let mut x: I32 = 0;
        while x < MAP_COLS {
            if (x + y) % 2 == 0 { map.Set(x, y, 1); }
            x = x + 1;
        }
        y = y + 1;
    }

    // Place some different tiles
    map.Set(5, 5, 2);
    map.Set(6, 5, 2);
    map.Set(7, 5, 2);
    map.Set(10, 10, 3);

    let mut cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);
    let mut showGrid: Bool = true;
    let mut selectedTile: I32 = 1;
    let mut lastClickX: I32 = -1;
    let mut lastClickY: I32 = -1;
    let mut lastClickId: I32 = 0;

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();

        // Camera movement
        if KeyDown(Key.W) { cam.y = cam.y - CAM_SPEED * dt; }
        if KeyDown(Key.S) { cam.y = cam.y + CAM_SPEED * dt; }
        if KeyDown(Key.A) { cam.x = cam.x - CAM_SPEED * dt; }
        if KeyDown(Key.D) { cam.x = cam.x + CAM_SPEED * dt; }

        // Zoom
        let scroll: F64 = GetMouseScroll();
        if scroll > 0.0 { cam.zoom = cam.zoom * 1.1; }
        if scroll < 0.0 { cam.zoom = cam.zoom * 0.9; }
        if cam.zoom < 0.2 { cam.zoom = 0.2; }
        if cam.zoom > 5.0 { cam.zoom = 5.0; }

        // Toggle grid
        if KeyPressed(Key.G) { showGrid = !showGrid; }

        // Toggle debug
        if KeyPressed(Key.F1) { DebugMode(!IsDebugMode()); }

        // Tile selection
        if KeyPressed(Key.Num1) { selectedTile = 1; }
        if KeyPressed(Key.Num2) { selectedTile = 2; }
        if KeyPressed(Key.Num3) { selectedTile = 3; }
        if KeyPressed(Key.Num4) { selectedTile = 4; }
        if KeyPressed(Key.Num5) { selectedTile = 5; }

        // World-space mouse for tile interaction
        let wmx: F64 = GetWorldMouseX(cam);
        let wmy: F64 = GetWorldMouseY(cam);
        let hover: TileCoord = map.WorldToTile(wmx, wmy);
        let inBounds: Bool = map.InBounds(hover.x, hover.y);

        // Place / remove tiles
        if MousePressed(Mouse.Left) && inBounds {
            lastClickX = hover.x;
            lastClickY = hover.y;
            lastClickId = map.Get(hover.x, hover.y);
            map.Set(hover.x, hover.y, selectedTile);
            lastClickId = selectedTile;
        }
        if MousePressed(Mouse.Right) && inBounds {
            lastClickX = hover.x;
            lastClickY = hover.y;
            lastClickId = 0;
            map.Set(hover.x, hover.y, 0);
        }

        // ── Render ──
        ClearBackground(0.15, 0.15, 0.25);

        BeginCamera2D(cam);

        // Draw tiles
        let mut ty: I32 = 0;
        while ty < MAP_ROWS {
            let mut tx: I32 = 0;
            while tx < MAP_COLS {
                let id: I32 = map.Get(tx, ty);
                if id > 0 {
                    let px: F32 = (tx * TILE_W) as F32;
                    let py: F32 = (ty * TILE_H) as F32;
                    let tw: F32 = TILE_W as F32;
                    let th: F32 = TILE_H as F32;

                    if id == 1 { DrawRectangle(px, py, tw, th, 0.2, 0.6, 0.2, 1.0); }
                    if id == 2 { DrawRectangle(px, py, tw, th, 0.6, 0.5, 0.3, 1.0); }
                    if id == 3 { DrawRectangle(px, py, tw, th, 0.3, 0.4, 0.8, 1.0); }
                    if id == 4 { DrawRectangle(px, py, tw, th, 0.8, 0.3, 0.3, 1.0); }
                    if id == 5 { DrawRectangle(px, py, tw, th, 0.7, 0.5, 0.8, 1.0); }
                }
                tx = tx + 1;
            }
            ty = ty + 1;
        }

        // Grid overlay — draw as thin rectangles (more reliable than DrawLine)
        if showGrid {
            let gw: F32 = (MAP_COLS * TILE_W) as F32;
            let gh: F32 = (MAP_ROWS * TILE_H) as F32;
            // Vertical lines
            let mut gx: I32 = 0;
            while gx <= MAP_COLS {
                let lx: F32 = (gx * TILE_W) as F32;
                DrawRectangle(lx, 0.0, 1.0, gh, 1.0, 1.0, 1.0, 0.3);
                gx = gx + 1;
            }
            // Horizontal lines
            let mut gy: I32 = 0;
            while gy <= MAP_ROWS {
                let ly: F32 = (gy * TILE_H) as F32;
                DrawRectangle(0.0, ly, gw, 1.0, 1.0, 1.0, 0.3);
                gy = gy + 1;
            }
        }

        // Highlight hovered tile with a bright yellow outline
        if inBounds {
            let hx: F32 = (hover.x * TILE_W) as F32;
            let hy: F32 = (hover.y * TILE_H) as F32;
            let tw: F32 = TILE_W as F32;
            let th: F32 = TILE_H as F32;
            // Semi-transparent yellow fill
            DrawRectangle(hx, hy, tw, th, 1.0, 1.0, 0.0, 0.2);
            // Bright yellow border
            DrawRectangle(hx, hy, tw, 2.0, 1.0, 1.0, 0.0, 1.0);
            DrawRectangle(hx, hy + th - 2.0, tw, 2.0, 1.0, 1.0, 0.0, 1.0);
            DrawRectangle(hx, hy, 2.0, th, 1.0, 1.0, 0.0, 1.0);
            DrawRectangle(hx + tw - 2.0, hy, 2.0, th, 1.0, 1.0, 0.0, 1.0);
        }

        // Highlight last clicked tile with a cyan outline
        if lastClickX >= 0 {
            let cx: F32 = (lastClickX * TILE_W) as F32;
            let cy: F32 = (lastClickY * TILE_H) as F32;
            let tw: F32 = TILE_W as F32;
            let th: F32 = TILE_H as F32;
            DrawRectangle(cx, cy, tw, 2.0, 0.0, 1.0, 1.0, 1.0);
            DrawRectangle(cx, cy + th - 2.0, tw, 2.0, 0.0, 1.0, 1.0, 1.0);
            DrawRectangle(cx, cy, 2.0, th, 0.0, 1.0, 1.0, 1.0);
            DrawRectangle(cx + tw - 2.0, cy, 2.0, th, 0.0, 1.0, 1.0, 1.0);
        }

        EndCamera2D();

        // ── HUD (screen-space) ──
        DrawText("Tilemap Test", 10.0, 10.0, 20, WHITE);
        DrawText("WASD: pan | Scroll: zoom | G: grid | F1: debug", 10.0, 35.0, 14, GRAY);
        DrawText("Left click: place | Right click: erase | 1-5: select tile", 10.0, 52.0, 14, GRAY);

        // Show selected tile preview
        DrawRectangle(10.0, 75.0, 20.0, 20.0, 0.3, 0.3, 0.3, 1.0);
        if selectedTile == 1 { DrawRectangle(12.0, 77.0, 16.0, 16.0, 0.2, 0.6, 0.2, 1.0); }
        if selectedTile == 2 { DrawRectangle(12.0, 77.0, 16.0, 16.0, 0.6, 0.5, 0.3, 1.0); }
        if selectedTile == 3 { DrawRectangle(12.0, 77.0, 16.0, 16.0, 0.3, 0.4, 0.8, 1.0); }
        if selectedTile == 4 { DrawRectangle(12.0, 77.0, 16.0, 16.0, 0.8, 0.3, 0.3, 1.0); }
        if selectedTile == 5 { DrawRectangle(12.0, 77.0, 16.0, 16.0, 0.7, 0.5, 0.8, 1.0); }
        DrawText("Selected: " + ToString(selectedTile), 36.0, 78.0, 14, WHITE);

        // Show hover info
        if inBounds {
            let hoverId: I32 = map.Get(hover.x, hover.y);
            DrawText("Hover: (" + ToString(hover.x) + ", " + ToString(hover.y) + ") ID: " + ToString(hoverId), 10.0, 100.0, 16, YELLOW);
        } else {
            DrawText("Hover: out of bounds", 10.0, 100.0, 16, GRAY);
        }

        // Show last clicked tile
        if lastClickX >= 0 {
            DrawText("Clicked: (" + ToString(lastClickX) + ", " + ToString(lastClickY) + ") ID: " + ToString(lastClickId), 10.0, 122.0, 16, CYAN);
        }

        Present();
        PollEvents();
    }
}
