#pragma once
#include <cmath>
#if __has_include("node.hpp")
#include "node.hpp"
#else
#include "../node/node.hpp"
#endif

class Node2D : public Node {
public:
    float x = 0, y = 0;
    float scaleX = 1, scaleY = 1;
    float rotation = 0;
    float originX = 0.5f;
    float originY = 0.5f;

    Node2D(const std::string& name = "Node2D") : Node(name) {}

    void Move(float dx, float dy) { x += dx; y += dy; }

    float DrawX(float w) const { return x - w * originX; }
    float DrawY(float h) const { return y - h * originY; }

    // Both Draw and Update children use the same world-space transform baking.
    // This ensures colliders have correct world positions during both
    // Update (collision) and Draw (debug outlines).
    void DrawChildren() override {
        float rad  = rotation * 3.14159265f / 180.f;
        float cosR = std::cos(rad);
        float sinR = std::sin(rad);
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                float savedX  = c->x,      savedY  = c->y;
                float savedSX = c->scaleX, savedSY = c->scaleY;
                // Rotate child's local offset by parent rotation, then translate
                float lx = c->x * scaleX;
                float ly = c->y * scaleY;
                c->x      = x + lx * cosR - ly * sinR;
                c->y      = y + lx * sinR + ly * cosR;
                c->scaleX *= scaleX;
                c->scaleY *= scaleY;
                c->Draw();
                c->DrawChildren();
                c->x = savedX; c->y = savedY;
                c->scaleX = savedSX; c->scaleY = savedSY;
            } else {
                child->Draw();
                child->DrawChildren();
            }
        }
    }
    void UpdateChildren(float dt)  override { propagateToChildren(true, dt); }

private:
    void propagateToChildren(bool doUpdate, float dt) {
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                // Save local coords
                float lx = c->x, ly = c->y, lsx = c->scaleX, lsy = c->scaleY;
                // Bake world transform
                c->x      = x + lx * scaleX;
                c->y      = y + ly * scaleY;
                c->scaleX = lsx * scaleX;
                c->scaleY = lsy * scaleY;
                // Recurse
                if (doUpdate) { c->Update(dt); c->UpdateChildren(dt); }
                else          { c->Draw();     c->DrawChildren();     }
                // Restore
                c->x = lx; c->y = ly; c->scaleX = lsx; c->scaleY = lsy;
            } else {
                if (doUpdate) { child->Update(dt); child->UpdateChildren(dt); }
                else          { child->Draw();     child->DrawChildren();     }
            }
        }
    }
};
