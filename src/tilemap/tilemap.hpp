#pragma once
#include "../color/color.hpp"
#include "../window/window.hpp"

// -----------------------------------------------------------------------
// TileGrid -- lightweight grid overlay and tile coordinate helpers.
// Useful for tilemaps, level editors, and snapping.
//
// Usage:
//   TileGrid grid(32, 32);          // 32x32 pixel tiles
//   grid.DrawGrid(0, 0, 25, 19);    // draw 25x19 tile grid at world origin
//   auto [tx, ty] = grid.WorldToTile(mouseX, mouseY);  // get tile coords
//   auto [wx, wy] = grid.TileToWorld(tx, ty);           // get world pos
// -----------------------------------------------------------------------

struct TileGrid {
    int tileW = 32;
    int tileH = 32;

    TileGrid() = default;
    TileGrid(int tileWidth, int tileHeight)
        : tileW(tileWidth), tileH(tileHeight) {}

    // Draw a grid overlay in world space.
    // originX/Y = top-left world position of the grid
    // cols/rows  = number of tiles wide/tall to draw
    void DrawGrid(float originX, float originY,
                  int cols, int rows,
                  Color color = {0.3f, 0.3f, 0.3f, 0.4f}) const
    {
        float w = (float)(cols * tileW);
        float h = (float)(rows * tileH);

        // Vertical lines
        for (int x = 0; x <= cols; x++) {
            float lx = originX + x * tileW;
            DrawLine(lx, originY, lx, originY + h, color);
        }
        // Horizontal lines
        for (int y = 0; y <= rows; y++) {
            float ly = originY + y * tileH;
            DrawLine(originX, ly, originX + w, ly, color);
        }
    }

    // Draw grid with a highlighted tile at (highlightTX, highlightTY)
    void DrawGridHighlight(float originX, float originY,
                           int cols, int rows,
                           int highlightTX, int highlightTY,
                           Color gridColor  = {0.3f, 0.3f, 0.3f, 0.4f},
                           Color fillColor  = {1.0f, 1.0f, 0.0f, 0.25f},
                           Color borderColor = {1.0f, 1.0f, 0.0f, 0.9f}) const
    {
        DrawGrid(originX, originY, cols, rows, gridColor);

        if (highlightTX >= 0 && highlightTX < cols &&
            highlightTY >= 0 && highlightTY < rows) {
            float hx = originX + highlightTX * tileW;
            float hy = originY + highlightTY * tileH;
            DrawRectangle(hx, hy, (float)tileW, (float)tileH, fillColor);
            DrawRectangle(hx,   hy,                (float)tileW, 1.0f, borderColor);
            DrawRectangle(hx,   hy + tileH - 1.0f, (float)tileW, 1.0f, borderColor);
            DrawRectangle(hx,   hy,                1.0f, (float)tileH, borderColor);
            DrawRectangle(hx + tileW - 1.0f, hy,  1.0f, (float)tileH, borderColor);
        }
    }

    // Convert world position to tile coordinates
    // Returns {-1,-1} if outside the grid bounds (when cols/rows > 0)
    struct TileCoord { int x, y; };

    TileCoord WorldToTile(float worldX, float worldY,
                          float originX = 0, float originY = 0) const
    {
        int tx = (int)((worldX - originX) / tileW);
        int ty = (int)((worldY - originY) / tileH);
        if (worldX < originX) tx--;
        if (worldY < originY) ty--;
        return {tx, ty};
    }

    // Snap a world position to the nearest tile top-left corner
    struct WorldPos { float x, y; };

    WorldPos TileToWorld(int tileX, int tileY,
                         float originX = 0, float originY = 0) const
    {
        return {originX + tileX * tileW, originY + tileY * tileH};
    }

    // Snap world position to tile grid
    WorldPos Snap(float worldX, float worldY,
                  float originX = 0, float originY = 0) const
    {
        auto [tx, ty] = WorldToTile(worldX, worldY, originX, originY);
        return TileToWorld(tx, ty, originX, originY);
    }

    // Get center of a tile in world space
    WorldPos TileCenter(int tileX, int tileY,
                        float originX = 0, float originY = 0) const
    {
        return {originX + tileX * tileW + tileW * 0.5f,
                originY + tileY * tileH + tileH * 0.5f};
    }
};
