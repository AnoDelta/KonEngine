#pragma once
#include "node2d.hpp"
#include "../color/color.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include "../window/window.hpp"

// Forward declaration
class Collider2D;

enum class ColliderShape {
    Rectangle,
    Circle,
    Custom  // Convex polygon via points
};

class Collider2D : public Node2D {
public:
    // Shape
    ColliderShape shape = ColliderShape::Rectangle;
    float width  = 32.0f;   // Rectangle width  / Circle uses radius instead
    float height = 32.0f;   // Rectangle height
    float radius = 16.0f;   // Circle radius
    std::vector<glm::vec2> points; // Custom convex polygon (local space)

    // Layer/mask bitmask (Godot-style)
    // A collision only happens if: (a.layer & b.mask) || (b.layer & a.mask)
    uint32_t layer = 1;
    uint32_t mask  = 1;

    // Debug
    bool debugDraw = false;
    Color debugColor = { 0.0f, 1.0f, 0.0f, 0.8f };

    Collider2D(const std::string& name = "Collider2D") : Node2D(name) {}

    // --- Typed signals (Collider2D* = the other collider) ---
    void Connect(const std::string& signal, std::function<void(Collider2D*)> cb) {
        typedSignals[signal].push_back(cb);
    }

    void Emit(const std::string& signal, Collider2D* other) {
        auto it = typedSignals.find(signal);
        if (it != typedSignals.end())
            for (auto& cb : it->second)
                cb(other);
    }

    // Helper: get world-space AABB for broad phase
    void GetAABB(float& outX, float& outY, float& outW, float& outH) const {
        switch (shape) {
			case ColliderShape::Rectangle:
				outX = DrawX(width); outY = DrawY(height);
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

    // Get world-space polygon points (used by SAT)
    std::vector<glm::vec2> GetWorldPoints() const {
        std::vector<glm::vec2> wp;
        switch (shape) {
			case ColliderShape::Rectangle: {
				float dx = DrawX(width);
				float dy = DrawY(height);
				wp = {
					{ dx,         dy          },
					{ dx + width, dy          },
					{ dx + width, dy + height },
					{ dx,         dy + height }
				};
				break;
}
            case ColliderShape::Circle:
                // Circles handled separately in SAT — return center as single point
                wp.push_back({ x, y });
                break;
            case ColliderShape::Custom:
                for (auto& p : points)
                    wp.push_back({ x + p.x, y + p.y });
                break;
        }
        return wp;
    }

	void Draw() override {
		if (!debugDraw) return;
		float dx = DrawX(width);
		float dy = DrawY(height);
		switch (shape) {
			case ColliderShape::Rectangle:
				DrawRectangle(dx, dy, width, height, debugColor);
				break;
			case ColliderShape::Circle:
				DrawCircle(x, y, radius, debugColor); // circle already draws from center
				break;
			case ColliderShape::Custom: {
				if (points.size() < 2) break;
				for (size_t i = 0; i < points.size(); i++) {
					glm::vec2 a = points[i];
					glm::vec2 b = points[(i + 1) % points.size()];
					DrawLine(x + a.x, y + a.y, x + b.x, y + b.y, debugColor);
				}
				break;
			}
		}
	}

private:
    std::unordered_map<std::string, std::vector<std::function<void(Collider2D*)>>> typedSignals;
};

