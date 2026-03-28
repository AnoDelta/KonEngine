#pragma once
#include "node.hpp"

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

    // Push this node's world transform down to all Node2D children.
    // Called by Scene::Update() before collision checks so colliders
    // have correct world-space positions.
    void ApplyTransformToChildren() {
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                c->x = x + c->x * scaleX;
                c->y = y + c->y * scaleY;
                c->ApplyTransformToChildren();
            }
        }
    }

    // Restore children back to local space after collision checks.
    void RestoreChildrenLocal() {
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                c->RestoreChildrenLocal();
                c->x = (c->x - x) / (scaleX != 0.0f ? scaleX : 1.0f);
                c->y = (c->y - y) / (scaleY != 0.0f ? scaleY : 1.0f);
            }
        }
    }

    void UpdateChildren(float dt) override {
        for (auto& child : children) {
            if (!child->active) continue;
            if (auto* c = dynamic_cast<Node2D*>(child.get())) {
                float savedX  = c->x,      savedY  = c->y;
                float savedSX = c->scaleX, savedSY = c->scaleY;
                c->x      = x + c->x * scaleX;
                c->y      = y + c->y * scaleY;
                c->scaleX *= scaleX;
                c->scaleY *= scaleY;
                c->Update(dt);
                c->UpdateChildren(dt);
                c->x      = savedX;  c->y      = savedY;
                c->scaleX = savedSX; c->scaleY = savedSY;
            } else {
                child->Update(dt);
                child->UpdateChildren(dt);
            }
        }
    }

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
                c->x      = savedX;  c->y      = savedY;
                c->scaleX = savedSX; c->scaleY = savedSY;
            } else {
                child->Draw();
                child->DrawChildren();
            }
        }
    }
};
