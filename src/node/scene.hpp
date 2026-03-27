#pragma once
#include "node.hpp"

class Scene {
public:
    template<typename T, typename... Args>
    T* Add(const std::string& nodeName, Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        node->name = nodeName;
        T* ptr = node.get();
        nodes.push_back(std::move(node));
        return ptr;
    }

    void Remove(const std::string& nodeName) {
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
