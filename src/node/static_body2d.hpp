#pragma once
#include "node2d.hpp"
#include "collider2d.hpp"

// StaticBody2D — Immovable wall/platform.
// Add Collider2D children for collision shape. The colliders are automatically
// marked as solid + static so they block other bodies but never move.
class StaticBody2D : public Node2D {
public:
    StaticBody2D(const std::string& name = "StaticBody2D") : Node2D(name) {}

    void Ready() override {
        // Mark every existing collider child as solid + static
        MarkColliders();
    }

    // Override to also mark colliders added after Ready()
    void Update(float /*dt*/) override {}

    // Helper: add a collider child pre-configured for static collision
    Collider2D* AddCollider(const std::string& childName,
                            float w = 32.0f, float h = 32.0f) {
        auto* col = AddChild<Collider2D>(childName);
        col->width      = w;
        col->height     = h;
        col->solid      = true;
        col->staticBody = true;
        return col;
    }

private:
    void MarkColliders() {
        ForEachDescendant([](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n)) {
                col->solid      = true;
                col->staticBody = true;
            }
        });
    }
};
