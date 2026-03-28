#pragma once
#include "node.hpp"
#include "node2d.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"

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

        // Re-scan after Ready() for colliders added during it
        ptr->ForEachDescendant([this](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n))
                collisionWorld.Add(col);
        });

        return ptr;
    }

    void ScanColliders() {
        for (auto& node : nodes) {
            if (auto* col = dynamic_cast<Collider2D*>(node.get()))
                collisionWorld.Add(col);
            node->ForEachDescendant([this](Node* n) {
                if (auto* col = dynamic_cast<Collider2D*>(n))
                    collisionWorld.Add(col);
            });
        }
    }

    void Remove(const std::string& nodeName) {
        for (auto& node : nodes) {
            if (node->name == nodeName) {
                if (auto* col = dynamic_cast<Collider2D*>(node.get()))
                    collisionWorld.Remove(col);
                node->ForEachDescendant([this](Node* n) {
                    if (auto* col = dynamic_cast<Collider2D*>(n))
                        collisionWorld.Remove(col);
                });
            }
        }
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(),
                [&](const std::unique_ptr<Node>& n) {
                    return n->name == nodeName;
                }),
            nodes.end()
        );
    }

    Node* GetNode(const std::string& nodeName) {
        for (auto& node : nodes) {
            if (node->name == nodeName) return node.get();
            Node* found = node->GetNode(nodeName);
            if (found) return found;
        }
        return nullptr;
    }

    void Update(float dt) {
        // 1. Update all nodes -- moves parents around
        for (auto& node : nodes)
            if (node->active) {
                node->Update(dt);
                node->UpdateChildren(dt);
            }

        // 2. Push world transforms to children so colliders have
        //    correct world-space positions for collision checks
        for (auto& node : nodes)
            if (node->active)
                if (auto* n2d = dynamic_cast<Node2D*>(node.get()))
                    n2d->ApplyTransformToChildren();

        // 3. Collision checks -- all colliders now at world positions
        collisionWorld.Update();

        // 4. Restore children to local space for next frame
        for (auto& node : nodes)
            if (node->active)
                if (auto* n2d = dynamic_cast<Node2D*>(node.get()))
                    n2d->RestoreChildrenLocal();
    }

    void Draw() {
        for (auto& node : nodes)
            if (node->active) {
                node->Draw();
                node->DrawChildren();
            }

        if (IsDebugMode()) {
            auto drawIfCollider = [](Node* n) {
                if (auto* col = dynamic_cast<Collider2D*>(n)) {
                    bool wasDraw = col->debugDraw;
                    col->debugDraw = true;
                    col->Draw();
                    col->debugDraw = wasDraw;
                }
            };
            for (auto& node : nodes) {
                drawIfCollider(node.get());
                node->ForEachDescendant(drawIfCollider);
            }
        }
    }

private:
    std::vector<std::unique_ptr<Node>> nodes;
};
