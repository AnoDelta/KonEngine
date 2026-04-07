#include <engine>

// ═══════════════════════════════════════════════════════════════════════
// Tilemap Test — monolithic file to test tilemap and grid features
//
// Tests: Tilemap data storage, tile placement/removal, grid drawing,
// coordinate conversion, click detection, camera with tilemap,
// TileGrid overlay, tile ID display
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
    InitWindow(800, 600, "Tilemap Test", true);
    SetTargetFPS(60);

    // Create tilemap
    let mut map: Tilemap = Tilemap(MAP_COLS, MAP_ROWS, TILE_W, TILE_H);
    map.originX = 0.0;
    map.originY = 0.0;

    // Since we don't have a tileset image, we'll draw tiles manually
    // Fill a checkerboard pattern to test
    for y: I32 in 0..MAP_ROWS {
        for x: I32 in 0..MAP_COLS {
            if (x + y) % 2 == 0 { map.Set(x, y, 1); }
        }
    }

    // Place some different tiles
    map.Set(5, 5, 2);
    map.Set(6, 5, 2);
    map.Set(7, 5, 2);
    map.Set(10, 10, 3);

    let mut cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);
    let mut showGrid: Bool = true;
    let mut selectedTile: I32 = 1;

    // TileGrid for coordinate helpers
    let grid: TileGrid = TileGrid(TILE_W, TILE_H);

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
            map.Set(hover.x, hover.y, selectedTile);
        }
        if MousePressed(Mouse.Right) && inBounds {
            map.Set(hover.x, hover.y, 0);
        }

        // ── Render ──
        ClearBackground(0.06, 0.06, 0.1);

        BeginCamera2D(cam);

        // Draw tiles manually (no tileset texture in this test)
        for ty: I32 in 0..MAP_ROWS {
            for tx: I32 in 0..MAP_COLS {
                let id: I32 = map.Get(tx, ty);
                if id > 0 {
                    let px: F64 = (tx * TILE_W) as F64;
                    let py: F64 = (ty * TILE_H) as F64;
                    let tw: F64 = TILE_W as F64;
                    let th: F64 = TILE_H as F64;

                    if id == 1 { DrawRectangle(px, py, tw, th, Color(0.15, 0.4, 0.15, 1.0)); }
                    if id == 2 { DrawRectangle(px, py, tw, th, Color(0.4, 0.35, 0.25, 1.0)); }
                    if id == 3 { DrawRectangle(px, py, tw, th, Color(0.2, 0.3, 0.6, 1.0)); }
                    if id == 4 { DrawRectangle(px, py, tw, th, Color(0.6, 0.2, 0.2, 1.0)); }
                    if id == 5 { DrawRectangle(px, py, tw, th, Color(0.5, 0.4, 0.6, 1.0)); }
                }
            }
        }

        // Grid overlay
        if showGrid {
            grid.DrawGrid(0.0, 0.0, MAP_COLS, MAP_ROWS);
        }

        // Highlight hovered tile
        if inBounds {
            let hx: F64 = (hover.x * TILE_W) as F64;
            let hy: F64 = (hover.y * TILE_H) as F64;
            let tw: F64 = TILE_W as F64;
            let th: F64 = TILE_H as F64;
            DrawRectangle(hx, hy, tw, th, Color(1.0, 1.0, 0.0, 0.15));
            DrawLine(hx, hy, hx + tw, hy, YELLOW);
            DrawLine(hx, hy + th, hx + tw, hy + th, YELLOW);
            DrawLine(hx, hy, hx, hy + th, YELLOW);
            DrawLine(hx + tw, hy, hx + tw, hy + th, YELLOW);
        }

        EndCamera2D();

        // ── HUD (screen-space) ──
        DrawText("Tilemap Test", 10.0, 10.0, 20, WHITE);
        DrawText("WASD: pan | Scroll: zoom | G: grid | F1: debug", 10.0, 35.0, 14, GRAY);
        DrawText("Left click: place | Right click: erase | 1-5: select tile", 10.0, 52.0, 14, GRAY);

        // Show selected tile
        DrawRectangle(10.0, 75.0, 20.0, 20.0, Color(0.3, 0.3, 0.3, 1.0));
        if selectedTile == 1 { DrawRectangle(12.0, 77.0, 16.0, 16.0, Color(0.15, 0.4, 0.15, 1.0)); }
        if selectedTile == 2 { DrawRectangle(12.0, 77.0, 16.0, 16.0, Color(0.4, 0.35, 0.25, 1.0)); }
        if selectedTile == 3 { DrawRectangle(12.0, 77.0, 16.0, 16.0, Color(0.2, 0.3, 0.6, 1.0)); }
        if selectedTile == 4 { DrawRectangle(12.0, 77.0, 16.0, 16.0, Color(0.6, 0.2, 0.2, 1.0)); }
        if selectedTile == 5 { DrawRectangle(12.0, 77.0, 16.0, 16.0, Color(0.5, 0.4, 0.6, 1.0)); }

        if inBounds {
            let hid: I32 = map.Get(hover.x, hover.y);
        }

        Present();
        PollEvents();
    }
}
