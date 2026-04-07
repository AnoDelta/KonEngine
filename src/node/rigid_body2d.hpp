#pragma once
#include "node2d.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"
#include "../math/vector2.hpp"

// RigidBody2D — Physics-driven body with velocity and gravity.
// After moving each frame it resolves collisions with static bodies and
// zeroes the velocity component along the collision normal.
//
// Physics runs automatically each frame via Update(). If you override
// Update(), call RigidBody2D::Update(dt) or PhysicsStep(dt) manually.
class RigidBody2D : public Node2D {
public:
    Vector2 velocity = Vector2::Zero();
    float   gravity  = 980.0f;           // pixels / s^2, positive = downward
    bool    onFloor  = false;            // true if touching a static body below

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

    // Convenience: auto-generates collider name
    Collider2D* AddCollider(float w = 32.0f, float h = 32.0f) {
        return AddCollider("collider_" + std::to_string(m_colCounter++), w, h);
    }

    // Auto-run physics each frame
    void Update(float dt) override {
        PhysicsStep(dt);
    }

    // PhysicsStep — apply gravity, move, resolve collisions.
    // Uses the CollisionWorld set by Scene automatically.
    void PhysicsStep(float dt) {
        if (!_world) return;

        // 1. Apply gravity
        velocity.y += gravity * dt;

        // 2. Move by velocity * dt
        float dx = velocity.x * dt;
        float dy = velocity.y * dt;
        x += dx;
        y += dy;

        onFloor = false;

        // 3. Resolve overlaps with static bodies using fresh MTV checks
        ForEachDescendant([&](Node* n) {
            auto* col = dynamic_cast<Collider2D*>(n);
            if (!col || !col->active) return;

            // SweepResolve pushes the collider's local x/y — undo that
            // and apply the push to the parent body only.
            float oldCX = col->x, oldCY = col->y;
            glm::vec2 push = _world->SweepResolve(col);
            col->x = oldCX;
            col->y = oldCY;

            if (push.x != 0.0f || push.y != 0.0f) {
                x += push.x;
                y += push.y;

                // Cancel velocity along the push direction
                Vector2 normal = Vector2(push.x, push.y).Normalized();
                float dot = velocity.Dot(normal);
                if (dot < 0.0f) {
                    velocity.x -= normal.x * dot;
                    velocity.y -= normal.y * dot;
                }

                // Detect floor (push has upward component)
                if (push.y < -0.01f) onFloor = true;
            }
        });
    }

    // Backwards-compatible overload accepting explicit CollisionWorld
    void PhysicsUpdate(float dt, CollisionWorld& world) {
        _world = &world;
        PhysicsStep(dt);
    }

private:
    int m_colCounter = 0;
};
