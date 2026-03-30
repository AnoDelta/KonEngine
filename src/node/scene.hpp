#pragma once
#include "node.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"
#include <cmath>

bool IsDebugMode();

class Scene {
public:
    CollisionWorld collisionWorld;

    template<typename T, typename... Args>
    T* Add(const std::string& nodeName, Args&&... args) {
        auto node = std::make_unique<T>(nodeName, std::forward<Args>(args)...);
        node->name = nodeName;
        T* ptr = node.get();
        nodes.push_back(std::move(node));
        if (auto* col = dynamic_cast<Collider2D*>(ptr))
            collisionWorld.Add(col);
        ptr->ForEachDescendant([this](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n))
                collisionWorld.Add(col);
        });
        ptr->Ready();
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
        for (auto& node : nodes)
            if (node->active) {
                node->Draw();
                node->DrawChildren();
            }

        if (IsDebugMode()) {
            // World-space grid — scales with camera when called inside BeginCamera2D
            const float CELL = 64.f;
            const int RANGE = 40;
            float ext = RANGE * CELL;
            Color minor{0.15f, 0.15f, 0.25f, 1.f};
            Color axis {0.30f, 0.30f, 0.50f, 1.f};
            for (int i = -RANGE; i <= RANGE; i++) {
                float v = i * CELL;
                DrawLine(v, -ext, v,  ext, (i == 0) ? axis : minor);
                DrawLine(-ext, v, ext, v,  (i == 0) ? axis : minor);
            }
            // Auto-draw all collider outlines — Draw() handles world transform internally
            for (auto& node : nodes)
                if (node->active)
                    drawDebug(node.get());
        }
    }

private:
    std::vector<std::unique_ptr<Node>> nodes;

    void drawDebug(Node* node) {
        if (auto* col = dynamic_cast<Collider2D*>(node)) {
            bool was = col->debugDraw;
            col->debugDraw = true;
            col->Draw();
            col->debugDraw = was;
        }
        for (auto& child : node->getChildren())
            if (child->active)
                drawDebug(child.get());
    }
};
