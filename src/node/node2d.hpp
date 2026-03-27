#pragma once
#include "node.hpp"

class Node2D : public Node {
public:
    float x = 0, y = 0;
    float scaleX = 1, scaleY = 1;
    float rotation = 0;

    // Origin pivot: 0 = left/top, 0.5 = center, 1 = right/bottom
    // Defaults to center (Godot-style)
    float originX = 0.5f;
    float originY = 0.5f;

    Node2D(const std::string& name = "Node2D") : Node(name) {}

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    // Helpers used by subclasses to get the actual top-left draw position
    float DrawX(float w) const { return x - w * originX; }
    float DrawY(float h) const { return y - h * originY; }

    void DrawChildren() override {
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                float savedX  = c->x,      savedY  = c->y;
                float savedSX = c->scaleX, savedSY = c->scaleY;
                c->x      = x + c->x * scaleX;
                c->y      = y + c->y * scaleY;
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
};
