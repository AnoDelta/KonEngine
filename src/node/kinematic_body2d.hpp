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

    // Convenience: auto-generates collider name
    Collider2D* AddCollider(float w = 32.0f, float h = 32.0f) {
        return AddCollider("collider_" + std::to_string(m_colCounter++), w, h);
    }

    // MoveAndCollide — attempt to move by (dx, dy), resolve overlaps with
    // static bodies using fresh MTV checks (not stale contact lists).
    // Uses the CollisionWorld set by Scene automatically.
    Vector2 MoveAndCollide(float dx, float dy) {
        // 1. Apply the tentative move to the parent body
        x += dx;
        y += dy;

        if (!_world) return Vector2(dx, dy);

        Vector2 totalPush(0.0f, 0.0f);

        // 2. For each collider child, sweep-resolve against all statics.
        //    SweepResolve pushes the collider's LOCAL x/y, but we want to
        //    move the PARENT instead. So we undo the child push and apply
        //    it to the parent.
        ForEachDescendant([&](Node* n) {
            auto* mover = dynamic_cast<Collider2D*>(n);
            if (!mover || !mover->active) return;

            float oldX = mover->x, oldY = mover->y;
            glm::vec2 push = _world->SweepResolve(mover);
            // Undo the push on the child — we'll push the parent instead
            mover->x = oldX;
            mover->y = oldY;
            totalPush.x += push.x;
            totalPush.y += push.y;
        });

        // 3. Apply the push to the parent body only
        x += totalPush.x;
        y += totalPush.y;

        return Vector2(dx + totalPush.x, dy + totalPush.y);
    }

    // Overload accepting an explicit CollisionWorld (backwards compatible)
    Vector2 MoveAndCollide(float dx, float dy, CollisionWorld& world) {
        _world = &world;
        return MoveAndCollide(dx, dy);
    }

private:
    int m_colCounter = 0;
};
