#pragma once
#include "node2d.hpp"
#include "../color/color.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include "../window/window.hpp"

class Collider2D;

enum class ColliderShape {
    Rectangle,
    Circle,
    Custom
};

class Collider2D : public Node2D {
public:
    ColliderShape shape  = ColliderShape::Rectangle;
    float width  = 32.0f;
    float height = 32.0f;
    float radius = 16.0f;
    std::vector<glm::vec2> points;

    uint32_t layer = 1;
    uint32_t mask  = 1;

    // Debug visuals
    bool  debugDraw  = false;
    Color debugColor = { 0.0f, 1.0f, 0.0f, 0.9f };

    // Set by CollisionWorld each frame — true if currently overlapping anything
    bool touching = false;

    Collider2D(const std::string& name = "Collider2D") : Node2D(name) {
        // Colliders use top-left origin by default so position = top-left corner
        // when used as standalone nodes. As child nodes they inherit parent transform.
        originX = 0.0f;
        originY = 0.0f;
    }

    void Connect(const std::string& signal, std::function<void(Collider2D*)> cb) {
        typedSignals[signal].push_back(cb);
    }

    void Emit(const std::string& signal, Collider2D* other) {
        auto it = typedSignals.find(signal);
        if (it != typedSignals.end())
            for (auto& cb : it->second)
                cb(other);
    }

    void GetAABB(float& outX, float& outY, float& outW, float& outH) const {
        switch (shape) {
            case ColliderShape::Rectangle:
                outX = x; outY = y;
                outW = width; outH = height;
                break;
            case ColliderShape::Circle:
                outX = x - radius; outY = y - radius;
                outW = radius * 2;  outH = radius * 2;
                break;
            case ColliderShape::Custom: {
                if (points.empty()) { outX = x; outY = y; outW = 0; outH = 0; return; }
                float minX = points[0].x, maxX = points[0].x;
                float minY = points[0].y, maxY = points[0].y;
                for (auto& p : points) {
                    if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
                    if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
                }
                outX = x + minX; outY = y + minY;
                outW = maxX - minX; outH = maxY - minY;
                break;
            }
        }
    }

    std::vector<glm::vec2> GetWorldPoints() const {
        std::vector<glm::vec2> wp;
        // World position = parent position + our local offset
        float wx = x;
        float wy = y;
        if (parent) {
            auto* p2d = dynamic_cast<Node2D*>(parent);
            if (p2d) { wx += p2d->x; wy += p2d->y; }
        }
        switch (shape) {
            case ColliderShape::Rectangle:
                wp = {
                    { wx,         wy          },
                    { wx + width, wy          },
                    { wx + width, wy + height },
                    { wx,         wy + height }
                };
                break;
            case ColliderShape::Circle:
                wp.push_back({ wx, wy });
                break;
            case ColliderShape::Custom:
                for (auto& p : points)
                    wp.push_back({ wx + p.x, wy + p.y });
                break;
        }
        return wp;
    }

    // World-space position accounting for parent
    float worldX() const {
        if (parent) {
            auto* p2d = dynamic_cast<Node2D*>(parent);
            if (p2d) return x + p2d->x;
        }
        return x;
    }
    float worldY() const {
        if (parent) {
            auto* p2d = dynamic_cast<Node2D*>(parent);
            if (p2d) return y + p2d->y;
        }
        return y;
    }

    void Draw() override {
        if (!debugDraw) return;

        // Color: green normally, yellow when touching
        Color c = touching
            ? Color{ 1.0f, 0.8f, 0.0f, 1.0f }
            : debugColor;

        float wx = worldX();
        float wy = worldY();

        switch (shape) {
            case ColliderShape::Rectangle: {
                // Draw outline (4 lines) instead of filled rect
                float x2 = wx + width;
                float y2 = wy + height;
                DrawLine(wx, wy, x2, wy, c);   // top
                DrawLine(x2, wy, x2, y2, c);   // right
                DrawLine(x2, y2, wx, y2, c);   // bottom
                DrawLine(wx, y2, wx, wy, c);   // left
                // Faint fill so you can see the box
                Color fill = { c.r, c.g, c.b, touching ? 0.25f : 0.12f };
                DrawRectangle(wx, wy, width, height, fill);
                break;
            }
            case ColliderShape::Circle:
                DrawCircle(wx, wy, radius, { c.r, c.g, c.b, 0.3f });
                // Approximate circle outline with lines
                {
                    const int segs = 16;
                    for (int i = 0; i < segs; i++) {
                        float a0 = (float)i     / segs * 6.2831853f;
                        float a1 = (float)(i+1) / segs * 6.2831853f;
                        DrawLine(wx + cosf(a0)*radius, wy + sinf(a0)*radius,
                                 wx + cosf(a1)*radius, wy + sinf(a1)*radius, c);
                    }
                }
                break;
            case ColliderShape::Custom:
                if (points.size() < 2) break;
                for (size_t i = 0; i < points.size(); i++) {
                    glm::vec2 a = points[i];
                    glm::vec2 b = points[(i+1) % points.size()];
                    DrawLine(wx+a.x, wy+a.y, wx+b.x, wy+b.y, c);
                }
                break;
        }
    }

private:
    std::unordered_map<std::string, std::vector<std::function<void(Collider2D*)>>> typedSignals;
};
