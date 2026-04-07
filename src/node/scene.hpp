#pragma once
#include "node.hpp"
#include "collider2d.hpp"
#include "static_body2d.hpp"
#include "kinematic_body2d.hpp"
#include "rigid_body2d.hpp"
#include "../collision/collision_world.hpp"
#include <cmath>
#include <functional>
#include "camera_node2d.hpp"

bool IsDebugMode();

// Auto-mark collider solid/static based on parent body type.
// Called when colliders are added to body nodes via Ready() or later.
static inline void AutoMarkCollider(Collider2D* col) {
    Node* p = col->parent;
    while (p) {
        if (dynamic_cast<StaticBody2D*>(p)) {
            col->solid = true;
            col->staticBody = true;
            return;
        }
        if (dynamic_cast<KinematicBody2D*>(p) || dynamic_cast<RigidBody2D*>(p)) {
            col->solid = true;
            col->staticBody = false;
            return;
        }
        p = p->parent;
    }
}

class Scene {
public:
    CollisionWorld collisionWorld;

    template<typename T, typename... Args>
    T* Add(const std::string& nodeName, Args&&... args) {
        auto node = std::make_unique<T>(nodeName, std::forward<Args>(args)...);
        node->name = nodeName;
        T* ptr = node.get();
        // Give the node access to the collision world for physics queries
        ptr->_world = &collisionWorld;
        // Set up child-added callback so colliders added in Ready() get registered
        // and automatically marked solid/static based on their parent body type.
        ptr->_onChildAdded = [this](Node* n) {
            n->_world = &collisionWorld;
            if (auto* col = dynamic_cast<Collider2D*>(n)) {
                AutoMarkCollider(col);
                collisionWorld.Add(col);
            }
        };
        nodes.push_back(std::move(node));
        // Call Ready() first — sets positions, adds children
        ptr->Ready();
        // THEN register colliders — positions are correct, no false overlaps.
        // Also auto-mark solid/static in case Ready() didn't call super.
        if (auto* col = dynamic_cast<Collider2D*>(ptr))
            collisionWorld.Add(col);
        ptr->ForEachDescendant([this](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n)) {
                AutoMarkCollider(col);
                collisionWorld.Add(col);
            }
        });
        return ptr;
    }

    void Remove(const std::string& nodeName) {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->name == nodeName) {
                if (auto* col = dynamic_cast<Collider2D*>(it->get()))
                    collisionWorld.Remove(col);
                (*it)->ForEachDescendant([this](Node* n) {
                    if (auto* col = dynamic_cast<Collider2D*>(n))
                        collisionWorld.Remove(col);
                });
                nodes.erase(it);
                return;
            }
        }
    }

    Node* GetNode(const std::string& nodeName) {
        for (auto& n : nodes) {
            if (n->name == nodeName) return n.get();
            if (Node* found = n->GetNode(nodeName)) return found;
        }
        return nullptr;
    }

    void Scan() {
        for (auto& node : nodes)
            node->ForEachDescendant([this](Node* n) {
                if (auto* col = dynamic_cast<Collider2D*>(n)) {
                    collisionWorld.Remove(col);
                    collisionWorld.Add(col);
                }
            });
    }

    void Update(float dt) {
        collisionWorld.Update();
        for (auto& node : nodes)
            if (node->active) {
                node->Update(dt);
                node->UpdateChildren(dt);
            }
    }

    void Draw() {
        // Find active CameraNode2D — search recursively
        CameraNode2D* activeCam = nullptr;
        std::function<CameraNode2D*(Node*)> findCam = [&](Node* n) -> CameraNode2D* {
            if (!n->active) return nullptr;
            if (auto* cam = dynamic_cast<CameraNode2D*>(n))
                if (cam->current) return cam;
            for (auto& child : n->getChildren())
                if (auto* found = findCam(child.get())) return found;
            return nullptr;
        };
        for (auto& node : nodes) {
            if (!activeCam) activeCam = findCam(node.get());
        }

        if (activeCam) BeginCamera2D(activeCam->toCamera2D());

        for (auto& node : nodes)
            if (node->active) {
                node->Draw();
                node->DrawChildren();
            }

        if (activeCam) EndCamera2D();

        if (IsDebugMode()) {
            // Auto-draw all collider outlines
            for (auto& node : nodes)
                if (node->active)
                    drawDebug(node.get());
        }
    }

private:
    std::vector<std::unique_ptr<Node>> nodes;

    void drawDebug(Node* node, float parentX = 0, float parentY = 0) {
        float wx = parentX, wy = parentY;
        if (auto* n2d = dynamic_cast<Node2D*>(node)) {
            wx += n2d->x;
            wy += n2d->y;
        }
        if (auto* col = dynamic_cast<Collider2D*>(node)) {
            // Temporarily set world-space position for drawing
            float savedX = col->x, savedY = col->y;
            col->x = wx; col->y = wy;
            bool was = col->debugDraw;
            col->debugDraw = true;
            col->Draw();
            col->debugDraw = was;
            col->x = savedX; col->y = savedY;
        }
        for (auto& child : node->getChildren())
            if (child->active)
                drawDebug(child.get(), wx, wy);
    }
};
