#pragma once
#if __has_include("node2d.hpp")
#include "node2d.hpp"
#else
#include "../node/node2d.hpp"
#endif
#include "../color/color.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include "../window/window.hpp"
#include <cmath>

class Collider2D;
enum class ColliderShape { Rectangle, Circle, Custom };

class Collider2D : public Node2D {
public:
    ColliderShape shape  = ColliderShape::Rectangle;
    float width  = 32.0f;
    float height = 32.0f;
    float radius = 16.0f;
    std::vector<glm::vec2> points;

    uint32_t layer = 1;
    uint32_t mask  = 1;

    bool  debugDraw  = false;
    Color debugColor = { 0.0f, 1.0f, 0.0f, 0.9f };
    bool  touching   = false;

    // solid      = true  → blocks other solid colliders (depenetration applied)
    // staticBody = true  → never moved by depenetration (walls, floors)
    bool  solid      = false;
    bool  staticBody = false;

    Collider2D(const std::string& name = "Collider2D") : Node2D(name) {
        // Default center pivot — matches Node2D default.
        // Child colliders are positioned relative to parent pivot.
        originX = 0.5f;
        originY = 0.5f;
    }

    void Connect(const std::string& sig, std::function<void(Collider2D*)> cb) {
        typedSignals[sig].push_back(cb);
    }
    void Emit(const std::string& sig, Collider2D* other) {
        // Fire typed signal listeners (Connect() callbacks)
        auto it = typedSignals.find(sig);
        if (it != typedSignals.end())
            for (auto& cb : it->second) cb(other);

        // Also bubble up to the parent node's virtual lifecycle methods.
        // This is how KonScript OnCollisionEnter/Exit overrides work —
        // they live on the parent node, not on the collider itself.
        if (parent) {
            if (sig == "on_collision_enter") parent->OnCollisionEnter(other);
            else if (sig == "on_collision_exit") parent->OnCollisionExit(other);
        }
    }

    // Compute world-space pivot position by walking the parent chain.
    // Uses the same transform as DrawChildren/UpdateChildren:
    //   world = parent.worldPivot + local * parent.scale
    // scaleX sign flip (for facing left) is intentional — but we take abs for size.
    glm::vec2 computeWorldPivot() const {
        float wx = x, wy = y;
        Node* p = parent;
        while (p) {
            auto* p2d = dynamic_cast<Node2D*>(p);
            if (!p2d) break;
            wx = p2d->x + wx * p2d->scaleX;
            wy = p2d->y + wy * p2d->scaleY;
            p  = p2d->parent;
        }
        return { wx, wy };
    }

    // World-space top-left of the collider rectangle, accounting for origin.
    // We use abs(scale) for size so flipping doesn't move the box.
    glm::vec2 computeWorldTopLeft() const {
        auto pivot = computeWorldPivot();
        // Effective world scale (take abs so flip doesn't shift position)
        float wsx = 1.0f, wsy = 1.0f;
        Node* p = parent;
        while (p) {
            auto* p2d = dynamic_cast<Node2D*>(p);
            if (!p2d) break;
            wsx *= p2d->scaleX;
            wsy *= p2d->scaleY;
            p = p2d->parent;
        }
        wsx = std::fabs(wsx) * std::fabs(scaleX);
        wsy = std::fabs(wsy) * std::fabs(scaleY);
        float w = width  * wsx;
        float h = height * wsy;
        return { pivot.x - w * originX, pivot.y - h * originY };
    }

    glm::vec2 worldCenter() const {
        auto pivot = computeWorldPivot();
        return pivot; // center IS the pivot when originX/Y=0.5
    }

    std::vector<glm::vec2> GetWorldPoints() const {
        auto tl = computeWorldTopLeft();
        // Use actual world-scaled size
        float wsx = 1.0f, wsy = 1.0f;
        Node* p = parent;
        while (p) {
            auto* p2d = dynamic_cast<Node2D*>(p);
            if (!p2d) break;
            wsx *= p2d->scaleX;
            wsy *= p2d->scaleY;
            p = p2d->parent;
        }
        wsx = std::fabs(wsx) * std::fabs(scaleX);
        wsy = std::fabs(wsy) * std::fabs(scaleY);
        float w = width  * wsx;
        float h = height * wsy;

        std::vector<glm::vec2> pts;
        switch (shape) {
            case ColliderShape::Rectangle:
                pts = {{ tl.x,   tl.y   },
                       { tl.x+w, tl.y   },
                       { tl.x+w, tl.y+h },
                       { tl.x,   tl.y+h }};
                break;
            case ColliderShape::Circle:
                pts.push_back(worldCenter());
                break;
            case ColliderShape::Custom: {
                auto piv = computeWorldPivot();
                for (auto& pt : points)
                    pts.push_back({ piv.x + pt.x * wsx, piv.y + pt.y * wsy });
                break;
            }
        }
        return pts;
    }

    void Draw() override {
        if (!debugDraw) return;
        Color c = debugColor; c.a = 1.0f;
        if (IsColliding()) { c.r = 1.0f; c.g = 1.0f; c.b = 0.2f; }

        // x/y are already world-space here — Node2D::DrawChildren() transforms
        // them before calling Draw(). Do NOT call computeWorldPivot() again.
        float drawX = x - width  * originX;
        float drawY = y - height * originY;
        float w = width  * std::fabs(scaleX);
        float h = height * std::fabs(scaleY);

        switch (shape) {
            case ColliderShape::Rectangle: {
                float x2 = drawX+w, y2 = drawY+h;
                DrawRectangle(drawX, drawY, w, h,
                    {c.r, c.g, c.b, IsColliding() ? 0.25f : 0.1f});
                DrawLine(drawX, drawY, x2,    drawY, c);
                DrawLine(x2,   drawY, x2,    y2,    c);
                DrawLine(x2,   y2,   drawX,  y2,    c);
                DrawLine(drawX, y2,  drawX,  drawY, c);
                break;
            }
            case ColliderShape::Circle: {
                float r = radius * std::max(std::fabs(scaleX), std::fabs(scaleY));
                DrawCircle(x, y, r, {c.r, c.g, c.b, 0.15f});
                const int S = 20;
                for (int i = 0; i < S; i++) {
                    float a0 = (float)i/S*6.28318f, a1 = (float)(i+1)/S*6.28318f;
                    DrawLine(x+cosf(a0)*r, y+sinf(a0)*r,
                             x+cosf(a1)*r, y+sinf(a1)*r, c);
                }
                break;
            }
            case ColliderShape::Custom: {
                for (size_t i = 0; i < points.size(); i++) {
                    auto a = points[i], b = points[(i+1)%points.size()];
                    DrawLine(x+a.x*scaleX, y+a.y*scaleY,
                             x+b.x*scaleX, y+b.y*scaleY, c);
                }
                break;
            }
        }
    }


    // ── Contact query ─────────────────────────────────────────────────
    // Updated every frame by CollisionWorld.
    // IsColliding()  → true if touching at least one other collider.
    // GetContacts()  → the actual pointers so you can inspect them.
    std::vector<Collider2D*> contacts;
    bool IsColliding() const { return !contacts.empty(); }
    const std::vector<Collider2D*>& GetContacts() const { return contacts; }

private:
    std::unordered_map<std::string,
        std::vector<std::function<void(Collider2D*)>>> typedSignals;
};
