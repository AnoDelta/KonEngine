#pragma once
#include "../color/color.hpp"
#include "../window/window.hpp"
#include "../renderer/texture.hpp"
#include <vector>
#include <cmath>

// -----------------------------------------------------------------------
// Shared coordinate types
// -----------------------------------------------------------------------
struct TileCoord { int x, y; };
struct WorldPos  { float x, y; };

// -----------------------------------------------------------------------
// TileGrid -- lightweight orthogonal grid overlay and coordinate helpers.
// No tile data -- just visualization and coordinate math.
// -----------------------------------------------------------------------
struct TileGrid {
    int tileW = 32;
    int tileH = 32;

    TileGrid() = default;
    TileGrid(int tileWidth, int tileHeight)
        : tileW(tileWidth), tileH(tileHeight) {}

    void DrawGrid(float originX, float originY,
                  int cols, int rows,
                  Color color = {0.3f, 0.3f, 0.3f, 0.4f}) const
    {
        float w = (float)(cols * tileW);
        float h = (float)(rows * tileH);
        for (int x = 0; x <= cols; x++) {
            float lx = originX + x * tileW;
            DrawLine(lx, originY, lx, originY + h, color);
        }
        for (int y = 0; y <= rows; y++) {
            float ly = originY + y * tileH;
            DrawLine(originX, ly, originX + w, ly, color);
        }
    }

    void DrawGridHighlight(float originX, float originY,
                           int cols, int rows,
                           int highlightTX, int highlightTY,
                           Color gridColor   = {0.3f, 0.3f, 0.3f, 0.4f},
                           Color fillColor   = {1.0f, 1.0f, 0.0f, 0.25f},
                           Color borderColor = {1.0f, 1.0f, 0.0f, 0.9f}) const
    {
        DrawGrid(originX, originY, cols, rows, gridColor);
        if (highlightTX >= 0 && highlightTX < cols &&
            highlightTY >= 0 && highlightTY < rows) {
            float hx = originX + highlightTX * tileW;
            float hy = originY + highlightTY * tileH;
            DrawRectangle(hx, hy, (float)tileW, (float)tileH, fillColor);
            DrawLine(hx, hy, hx + tileW, hy, borderColor);
            DrawLine(hx, hy + tileH, hx + tileW, hy + tileH, borderColor);
            DrawLine(hx, hy, hx, hy + tileH, borderColor);
            DrawLine(hx + tileW, hy, hx + tileW, hy + tileH, borderColor);
        }
    }

    TileCoord WorldToTile(float worldX, float worldY,
                          float originX = 0, float originY = 0) const
    {
        int tx = (int)((worldX - originX) / tileW);
        int ty = (int)((worldY - originY) / tileH);
        if (worldX < originX) tx--;
        if (worldY < originY) ty--;
        return {tx, ty};
    }

    WorldPos TileToWorld(int tileX, int tileY,
                         float originX = 0, float originY = 0) const
    {
        return {originX + tileX * tileW, originY + tileY * tileH};
    }

    WorldPos Snap(float worldX, float worldY,
                  float originX = 0, float originY = 0) const
    {
        auto tc = WorldToTile(worldX, worldY, originX, originY);
        return TileToWorld(tc.x, tc.y, originX, originY);
    }

    WorldPos TileCenter(int tileX, int tileY,
                        float originX = 0, float originY = 0) const
    {
        return {originX + tileX * tileW + tileW * 0.5f,
                originY + tileY * tileH + tileH * 0.5f};
    }
};

// -----------------------------------------------------------------------
// Tilemap -- 2D tile data storage with tileset rendering.
//
// Stores a 2D grid of integer tile IDs. Tile ID 0 = empty (not drawn).
// Renders using a tileset texture (spritesheet) where tiles are arranged
// in a grid of tileW x tileH pixel cells.
//
// Usage:
//   Tilemap map(20, 15, 32, 32);          // 20x15 map, 32px tiles
//   map.tileset = LoadTexture("tiles.png");
//   map.Set(5, 3, 1);                     // place tile ID 1 at (5,3)
//   map.Draw(0, 0);                       // render at world origin
// -----------------------------------------------------------------------
class Tilemap {
public:
    int cols, rows;
    int tileW, tileH;
    float originX = 0, originY = 0;

    Texture tileset = {0, 0, 0};
    int tilesetCols = 0; // tiles per row in tileset (auto-calculated)

    Tilemap() : cols(0), rows(0), tileW(32), tileH(32) {}

    Tilemap(int cols, int rows, int tileW, int tileH)
        : cols(cols), rows(rows), tileW(tileW), tileH(tileH),
          tiles(cols * rows, 0) {}

    // --- Tile data access ---

    void Set(int x, int y, int tileId) {
        if (x >= 0 && x < cols && y >= 0 && y < rows)
            tiles[y * cols + x] = tileId;
    }

    int Get(int x, int y) const {
        if (x >= 0 && x < cols && y >= 0 && y < rows)
            return tiles[y * cols + x];
        return 0;
    }

    void Fill(int tileId) {
        for (auto& t : tiles) t = tileId;
    }

    void Clear() {
        for (auto& t : tiles) t = 0;
    }

    void Resize(int newCols, int newRows) {
        std::vector<int> newTiles(newCols * newRows, 0);
        int copyW = (newCols < cols) ? newCols : cols;
        int copyH = (newRows < rows) ? newRows : rows;
        for (int y = 0; y < copyH; y++)
            for (int x = 0; x < copyW; x++)
                newTiles[y * newCols + x] = tiles[y * cols + x];
        tiles = std::move(newTiles);
        cols = newCols;
        rows = newRows;
    }

    // --- Coordinate conversion ---

    TileCoord WorldToTile(float worldX, float worldY) const {
        int tx = (int)((worldX - originX) / tileW);
        int ty = (int)((worldY - originY) / tileH);
        if (worldX < originX) tx--;
        if (worldY < originY) ty--;
        return {tx, ty};
    }

    WorldPos TileToWorld(int tileX, int tileY) const {
        return {originX + tileX * tileW, originY + tileY * tileH};
    }

    WorldPos TileCenter(int tileX, int tileY) const {
        return {originX + tileX * tileW + tileW * 0.5f,
                originY + tileY * tileH + tileH * 0.5f};
    }

    bool InBounds(int tileX, int tileY) const {
        return tileX >= 0 && tileX < cols && tileY >= 0 && tileY < rows;
    }

    // --- Click detection ---

    // Returns tile at world position, or {-1,-1} if out of bounds
    TileCoord GetTileAt(float worldX, float worldY) const {
        auto tc = WorldToTile(worldX, worldY);
        if (!InBounds(tc.x, tc.y)) return {-1, -1};
        return tc;
    }

    // Returns tile ID at world position, or 0 if out of bounds
    int GetTileIdAt(float worldX, float worldY) const {
        auto tc = GetTileAt(worldX, worldY);
        if (tc.x < 0) return 0;
        return Get(tc.x, tc.y);
    }

    // --- Rendering ---

    void SetTileset(Texture tex) {
        tileset = tex;
        if (tileW > 0) tilesetCols = tex.width / tileW;
    }

    // Draw the full tilemap at its origin
    void Draw() const {
        if (tileset.id == 0 || tilesetCols <= 0) return;

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int id = tiles[y * cols + x];
                if (id <= 0) continue;

                // Tile ID 1 = first tile in tileset (row 0, col 0)
                int tid = id - 1;
                int srcCol = tid % tilesetCols;
                int srcRow = tid / tilesetCols;

                float srcX = (float)(srcCol * tileW);
                float srcY = (float)(srcRow * tileH);
                float dx = originX + x * tileW;
                float dy = originY + y * tileH;

                DrawTextureRec(tileset, dx, dy, (float)tileW, (float)tileH,
                               srcX, srcY, (float)tileW, (float)tileH);
            }
        }
    }

    // Draw a single tile from the tileset at a specific world position
    void DrawTileAt(int tileId, float worldX, float worldY) const {
        if (tileset.id == 0 || tilesetCols <= 0 || tileId <= 0) return;

        int tid = tileId - 1;
        int srcCol = tid % tilesetCols;
        int srcRow = tid / tilesetCols;

        float srcX = (float)(srcCol * tileW);
        float srcY = (float)(srcRow * tileH);

        DrawTextureRec(tileset, worldX, worldY, (float)tileW, (float)tileH,
                       srcX, srcY, (float)tileW, (float)tileH);
    }

    // Draw a debug grid overlay
    void DrawGrid(Color color = {0.3f, 0.3f, 0.3f, 0.4f}) const {
        float w = (float)(cols * tileW);
        float h = (float)(rows * tileH);
        for (int x = 0; x <= cols; x++) {
            float lx = originX + x * tileW;
            DrawLine(lx, originY, lx, originY + h, color);
        }
        for (int y = 0; y <= rows; y++) {
            float ly = originY + y * tileH;
            DrawLine(originX, ly, originX + w, ly, color);
        }
    }

private:
    std::vector<int> tiles;
};

// -----------------------------------------------------------------------
// IsometricGrid -- diamond-shaped tile grid for isometric games.
//
// Tile dimensions: tileW = diamond width, tileH = diamond height.
// Standard isometric: tileW = 2 * tileH (e.g., 64x32).
//
// Coordinate system:
//   Screen (0,0) tile is at (originX, originY).
//   X-axis goes down-right, Y-axis goes down-left.
//
// Usage:
//   IsometricGrid iso(64, 32);
//   auto [sx, sy] = iso.TileToScreen(3, 5);     // tile -> screen pos
//   auto [tx, ty] = iso.ScreenToTile(mouseX, mouseY);  // click -> tile
//   iso.DrawGrid(0, 200, 10, 10);               // draw diamond grid
// -----------------------------------------------------------------------
struct IsometricGrid {
    int tileW = 64;
    int tileH = 32;

    IsometricGrid() = default;
    IsometricGrid(int tileWidth, int tileHeight)
        : tileW(tileWidth), tileH(tileHeight) {}

    // Convert tile coordinates to screen position (top of diamond)
    WorldPos TileToScreen(int tileX, int tileY,
                          float originX = 0, float originY = 0) const
    {
        float sx = originX + (tileX - tileY) * (tileW / 2.0f);
        float sy = originY + (tileX + tileY) * (tileH / 2.0f);
        return {sx, sy};
    }

    // Convert screen position to tile coordinates
    TileCoord ScreenToTile(float screenX, float screenY,
                           float originX = 0, float originY = 0) const
    {
        float relX = screenX - originX;
        float relY = screenY - originY;

        float halfW = tileW / 2.0f;
        float halfH = tileH / 2.0f;

        // Inverse of the isometric transform
        float fx = (relX / halfW + relY / halfH) / 2.0f;
        float fy = (relY / halfH - relX / halfW) / 2.0f;

        return {(int)std::floor(fx), (int)std::floor(fy)};
    }

    // Get center of a tile in screen space
    WorldPos TileCenter(int tileX, int tileY,
                        float originX = 0, float originY = 0) const
    {
        auto pos = TileToScreen(tileX, tileY, originX, originY);
        return {pos.x + tileW / 2.0f, pos.y + tileH / 2.0f};
    }

    bool InBounds(int tileX, int tileY, int cols, int rows) const {
        return tileX >= 0 && tileX < cols && tileY >= 0 && tileY < rows;
    }

    // Draw isometric diamond grid
    void DrawGrid(float originX, float originY,
                  int cols, int rows,
                  Color color = {0.3f, 0.3f, 0.3f, 0.4f}) const
    {
        float halfW = tileW / 2.0f;
        float halfH = tileH / 2.0f;

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                auto pos = TileToScreen(x, y, originX, originY);
                float cx = pos.x + halfW;
                float cy = pos.y + halfH;

                // Diamond shape: 4 lines
                DrawLine(cx,         cy - halfH, cx + halfW, cy,         color); // top -> right
                DrawLine(cx + halfW, cy,         cx,         cy + halfH, color); // right -> bottom
                DrawLine(cx,         cy + halfH, cx - halfW, cy,         color); // bottom -> left
                DrawLine(cx - halfW, cy,         cx,         cy - halfH, color); // left -> top
            }
        }
    }

    // Draw grid with a highlighted tile
    void DrawGridHighlight(float originX, float originY,
                           int cols, int rows,
                           int highlightTX, int highlightTY,
                           Color gridColor   = {0.3f, 0.3f, 0.3f, 0.4f},
                           Color fillColor   = {1.0f, 1.0f, 0.0f, 0.25f},
                           Color borderColor = {1.0f, 1.0f, 0.0f, 0.9f}) const
    {
        DrawGrid(originX, originY, cols, rows, gridColor);

        if (InBounds(highlightTX, highlightTY, cols, rows)) {
            auto pos = TileToScreen(highlightTX, highlightTY, originX, originY);
            float halfW = tileW / 2.0f;
            float halfH = tileH / 2.0f;
            float cx = pos.x + halfW;
            float cy = pos.y + halfH;

            // Fill diamond (approximate with rectangle - exact fill would need triangles)
            // For now, draw highlighted border
            DrawLine(cx,         cy - halfH, cx + halfW, cy,         borderColor);
            DrawLine(cx + halfW, cy,         cx,         cy + halfH, borderColor);
            DrawLine(cx,         cy + halfH, cx - halfW, cy,         borderColor);
            DrawLine(cx - halfW, cy,         cx,         cy - halfH, borderColor);
        }
    }
};

// -----------------------------------------------------------------------
// IsoTilemap -- isometric tilemap with tile data storage and rendering.
//
// Combines IsometricGrid coordinate math with Tilemap-style tile storage.
// Tile sprites in the tileset are rectangular (tileW x tileH) and drawn
// at diamond-projected screen positions.
//
// Usage:
//   IsoTilemap map(10, 10, 64, 32);
//   map.SetTileset(LoadTexture("iso_tiles.png"));
//   map.Set(3, 5, 1);   // place tile ID 1
//   map.Draw();          // renders all tiles at correct iso positions
//
//   // Click detection
//   auto tc = map.ScreenToTile(mouseX, mouseY);
//   if (map.InBounds(tc.x, tc.y)) { map.Set(tc.x, tc.y, 2); }
// -----------------------------------------------------------------------
class IsoTilemap {
public:
    int cols, rows;
    int tileW, tileH;
    float originX = 0, originY = 0;

    Texture tileset = {0, 0, 0};
    int tilesetCols = 0;

    IsoTilemap() : cols(0), rows(0), tileW(64), tileH(32) {}

    IsoTilemap(int cols, int rows, int tileW, int tileH)
        : cols(cols), rows(rows), tileW(tileW), tileH(tileH),
          tiles(cols * rows, 0) {}

    // --- Tile data ---

    void Set(int x, int y, int tileId) {
        if (x >= 0 && x < cols && y >= 0 && y < rows)
            tiles[y * cols + x] = tileId;
    }

    int Get(int x, int y) const {
        if (x >= 0 && x < cols && y >= 0 && y < rows)
            return tiles[y * cols + x];
        return 0;
    }

    void Fill(int tileId) { for (auto& t : tiles) t = tileId; }
    void Clear() { for (auto& t : tiles) t = 0; }

    void SetTileset(Texture tex) {
        tileset = tex;
        if (tileW > 0) tilesetCols = tex.width / tileW;
    }

    bool InBounds(int tileX, int tileY) const {
        return tileX >= 0 && tileX < cols && tileY >= 0 && tileY < rows;
    }

    // --- Coordinate conversion ---

    WorldPos TileToScreen(int tileX, int tileY) const {
        float sx = originX + (tileX - tileY) * (tileW / 2.0f);
        float sy = originY + (tileX + tileY) * (tileH / 2.0f);
        return {sx, sy};
    }

    TileCoord ScreenToTile(float screenX, float screenY) const {
        float relX = screenX - originX;
        float relY = screenY - originY;
        float halfW = tileW / 2.0f;
        float halfH = tileH / 2.0f;
        float fx = (relX / halfW + relY / halfH) / 2.0f;
        float fy = (relY / halfH - relX / halfW) / 2.0f;
        return {(int)std::floor(fx), (int)std::floor(fy)};
    }

    WorldPos TileCenter(int tileX, int tileY) const {
        auto pos = TileToScreen(tileX, tileY);
        return {pos.x + tileW / 2.0f, pos.y + tileH / 2.0f};
    }

    // --- Click detection ---

    TileCoord GetTileAt(float screenX, float screenY) const {
        auto tc = ScreenToTile(screenX, screenY);
        if (!InBounds(tc.x, tc.y)) return {-1, -1};
        return tc;
    }

    int GetTileIdAt(float screenX, float screenY) const {
        auto tc = GetTileAt(screenX, screenY);
        if (tc.x < 0) return 0;
        return Get(tc.x, tc.y);
    }

    // --- Rendering ---

    // Draw all tiles at their isometric screen positions.
    // Tiles are drawn back-to-front (top-left to bottom-right) for correct overlap.
    void Draw() const {
        if (tileset.id == 0 || tilesetCols <= 0) return;

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int id = tiles[y * cols + x];
                if (id <= 0) continue;

                int tid = id - 1;
                int srcCol = tid % tilesetCols;
                int srcRow = tid / tilesetCols;

                float srcX = (float)(srcCol * tileW);
                float srcY = (float)(srcRow * tileH);

                auto pos = TileToScreen(x, y);
                DrawTextureRec(tileset, pos.x, pos.y,
                               (float)tileW, (float)tileH,
                               srcX, srcY, (float)tileW, (float)tileH);
            }
        }
    }

    // Draw a single tile at a specific isometric grid position
    void DrawTileAt(int tileId, int tileX, int tileY) const {
        if (tileset.id == 0 || tilesetCols <= 0 || tileId <= 0) return;

        int tid = tileId - 1;
        int srcCol = tid % tilesetCols;
        int srcRow = tid / tilesetCols;

        float srcX = (float)(srcCol * tileW);
        float srcY = (float)(srcRow * tileH);

        auto pos = TileToScreen(tileX, tileY);
        DrawTextureRec(tileset, pos.x, pos.y,
                       (float)tileW, (float)tileH,
                       srcX, srcY, (float)tileW, (float)tileH);
    }

    // Draw diamond grid overlay
    void DrawGrid(Color color = {0.3f, 0.3f, 0.3f, 0.4f}) const {
        float halfW = tileW / 2.0f;
        float halfH = tileH / 2.0f;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                auto pos = TileToScreen(x, y);
                float cx = pos.x + halfW;
                float cy = pos.y + halfH;
                DrawLine(cx, cy - halfH, cx + halfW, cy, color);
                DrawLine(cx + halfW, cy, cx, cy + halfH, color);
                DrawLine(cx, cy + halfH, cx - halfW, cy, color);
                DrawLine(cx - halfW, cy, cx, cy - halfH, color);
            }
        }
    }

private:
    std::vector<int> tiles;
};
