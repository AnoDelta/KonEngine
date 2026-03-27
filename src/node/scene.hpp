#pragma once
#include "node.hpp"
#include "collider2d.hpp"
#include "../collision/collision_world.hpp"

class Scene {
public:
    CollisionWorld collisionWorld; // accessible if you want manual queries too

    template<typename T, typename... Args>
    T* Add(const std::string& nodeName, Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        node->name = nodeName;
        T* ptr = node.get();
        nodes.push_back(std::move(node));

        // Register the node itself if it's a collider
        if (auto* col = dynamic_cast<Collider2D*>(ptr))
            collisionWorld.Add(col);

        // Also register any Collider2D children already attached before scene add
        ptr->ForEachDescendant([this](Node* n) {
            if (auto* col = dynamic_cast<Collider2D*>(n))
                collisionWorld.Add(col);
        });

        return ptr;
    }

    void Remove(const std::string& nodeName) {
        for (auto& node : nodes) {
            if (node->name == nodeName) {
                // Unregister the node itself
                if (auto* col = dynamic_cast<Collider2D*>(node.get()))
                    collisionWorld.Remove(col);
                // Unregister any collider descendants
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
        // Run collision checks first
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
    }

private:
    std::vector<std::unique_ptr<Node>> nodes;
};
