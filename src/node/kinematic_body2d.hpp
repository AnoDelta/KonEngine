#pragma once
#include "node2d.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"
#include "../math/vector2.hpp"

// KinematicBody2D — Player-controlled body that slides along walls.
// Owns a Collider2D child for its shape.  Use MoveAndCollide() to move
// with automatic wall sliding / depenetration against StaticBody2D colliders.
class KinematicBody2D : public Node2D {
public:
    KinematicBody2D(const std::string& name = "KinematicBody2D") : Node2D(name) {}

    void Ready() override {
        // Mark collider children as solid (but NOT static — they can be pushed)
        ForEachDescendant([](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n)) {
                col->solid      = true;
                col->staticBody = false;
            }
        });
    }

    // Helper: add a collider child pre-configured for kinematic collision
    Collider2D* AddCollider(const std::string& childName,
                            float w = 32.0f, float h = 32.0f) {
        auto* col = AddChild<Collider2D>(childName);
        col->width      = w;
        col->height     = h;
        col->solid      = true;
        col->staticBody = false;
        return col;
    }

    // MoveAndCollide — attempt to move by (dx, dy), resolve overlaps with
    // static bodies, and return the actual displacement applied.
    // `world` must be the scene's CollisionWorld so we can query colliders.
    Vector2 MoveAndCollide(float dx, float dy, CollisionWorld& world) {
        // 1. Apply the tentative move
        x += dx;
        y += dy;

        Vector2 totalPush(0.0f, 0.0f);

        // 2. For each of our collider children, check against all static colliders
        ForEachDescendant([&](Node* n) {
            auto* mover = dynamic_cast<Collider2D*>(n);
            if (!mover || !mover->active) return;

            // Resolve may need multiple iterations (corner cases)
            for (int iter = 0; iter < 4; ++iter) {
                bool pushed = false;
                for (auto* other : mover->GetContacts()) {
                    if (!other->staticBody) continue;
                    Vector2 push = world.ResolveOverlap(mover, other);
                    totalPush.x += push.x;
                    totalPush.y += push.y;
                    if (push.x != 0.0f || push.y != 0.0f) pushed = true;
                }
                // Also do a fresh MTV check in case contacts list is stale
                // (contacts are rebuilt each CollisionWorld::Update)
                if (!pushed) break;
            }
        });

        // The actual movement is the requested delta + any push-back
        return Vector2(dx + totalPush.x, dy + totalPush.y);
    }
};
