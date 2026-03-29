#pragma once
#include "node.hpp"
#include "node2d.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"
#include "../window/window.hpp"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

class Scene {
public:
    CollisionWorld collisionWorld;

    template<typename T, typename... Args>
    T* Add(const std::string& name, Args&&... args) {
        auto node = std::make_unique<T>(name);
        T* ptr = node.get();

        // Set up the auto-registration callback BEFORE Ready() so any children
        // added during Ready() (or any time after) get registered automatically.
        // This means scene.Scan() is only needed for nodes added to the scene
        // after their parent was already added — e.g. dynamic scene.add() calls.
        ptr->_onChildAdded = [this](Node* n) {
            registerNode(n);
        };

        nodes.push_back(std::move(node));

        // Call Ready — children added here will auto-register via _onChildAdded
        ptr->Ready();

        // Register this node itself if it's a collider
        if (auto* col = dynamic_cast<Collider2D*>(ptr))
            collisionWorld.Add(col);

        // Register any collider children added during Ready()
        ptr->ForEachDescendant([this](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n))
                collisionWorld.Add(col);
        });

        return ptr;
    }

    void Remove(const std::string& name) {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->name == name) {
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

    Node* GetNode(const std::string& name) {
        for (auto& n : nodes) {
            if (n->name == name) return n.get();
            Node* found = n->GetNode(name);
            if (found) return found;
        }
        return nullptr;
    }

    // Re-scan the whole tree and rebuild the collision world.
    // Call after dynamically adding children outside of Add().
    void Scan() {
        collisionWorld.Clear();
        for (auto& node : nodes) {
            if (auto* col = dynamic_cast<Collider2D*>(node.get()))
                collisionWorld.Add(col);
            node->ForEachDescendant([this](Node* n) {
                if (auto* col = dynamic_cast<Collider2D*>(n))
                    collisionWorld.Add(col);
            });
        }
    }

    void Update(float dt) {
        // 1. Update all nodes (Node2D::UpdateChildren bakes world transforms)
        for (auto& node : nodes)
            if (node->active) {
                node->Update(dt);
                node->UpdateChildren(dt);
            }

        // 2. Run collision AFTER updates so world positions are current
        collisionWorld.Update();
    }

    void Draw() {
        for (auto& node : nodes)
            if (node->active) {
                node->Draw();
                node->DrawChildren();
            }

        // Debug: draw all collider outlines
        if (IsDebugMode()) {
            for (auto& node : nodes) {
                if (!node->active) continue;
                auto tryDraw = [](Node* n) {
                    if (auto* col = dynamic_cast<Collider2D*>(n)) {
                        bool was = col->debugDraw;
                        col->debugDraw = true;
                        col->Draw();
                        col->debugDraw = was;
                    }
                };
                tryDraw(node.get());
                node->ForEachDescendant(tryDraw);
            }
        }
    }

private:
    std::vector<std::unique_ptr<Node>> nodes;

    // Register a node and all its descendants with the collision world.
    // Called automatically when children are added via AddChild.
    void registerNode(Node* n) {
        if (auto* col = dynamic_cast<Collider2D*>(n))
            collisionWorld.Add(col);
        n->_onChildAdded = [this](Node* child) { registerNode(child); };
        n->ForEachDescendant([this](Node* d) {
            if (auto* col = dynamic_cast<Collider2D*>(d))
                collisionWorld.Add(col);
        });
    }
};
