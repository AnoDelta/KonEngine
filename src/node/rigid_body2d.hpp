#pragma once
#include "node2d.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"
#include "../math/vector2.hpp"

// RigidBody2D — Physics-driven body with velocity and gravity.
// After moving each frame it resolves collisions with static bodies and
// zeroes the velocity component along the collision normal.
class RigidBody2D : public Node2D {
public:
    Vector2 velocity = Vector2::Zero();
    float   gravity  = 980.0f;           // pixels / s^2, positive = downward

    RigidBody2D(const std::string& name = "RigidBody2D") : Node2D(name) {}

    void Ready() override {
        ForEachDescendant([](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n)) {
                col->solid      = true;
                col->staticBody = false;
            }
        });
    }

    // Helper: add a collider child pre-configured for rigid body collision
    Collider2D* AddCollider(const std::string& childName,
                            float w = 32.0f, float h = 32.0f) {
        auto* col = AddChild<Collider2D>(childName);
        col->width      = w;
        col->height     = h;
        col->solid      = true;
        col->staticBody = false;
        return col;
    }

    // Call once per frame with dt = GetDeltaTime() and the scene's CollisionWorld.
    void PhysicsUpdate(float dt, CollisionWorld& world) {
        // 1. Apply gravity
        velocity.y += gravity * dt;

        // 2. Move by velocity * dt
        float dx = velocity.x * dt;
        float dy = velocity.y * dt;
        x += dx;
        y += dy;

        // 3. Resolve overlaps with static bodies and cancel velocity on collision axis
        ForEachDescendant([&](Node* n) {
            auto* col = dynamic_cast<Collider2D*>(n);
            if (!col || !col->active) return;

            for (int iter = 0; iter < 4; ++iter) {
                bool pushed = false;
                for (auto* other : col->GetContacts()) {
                    if (!other->staticBody) continue;
                    Vector2 push = world.ResolveOverlap(col, other);
                    if (push.x != 0.0f || push.y != 0.0f) {
                        // Zero velocity along the push (collision normal) direction
                        Vector2 normal = push.Normalized();
                        float dot = velocity.Dot(normal);
                        if (dot < 0.0f) {
                            // Only cancel if velocity is going INTO the wall
                            velocity.x -= normal.x * dot;
                            velocity.y -= normal.y * dot;
                        }
                        pushed = true;
                    }
                }
                if (!pushed) break;
            }
        });
    }
};
