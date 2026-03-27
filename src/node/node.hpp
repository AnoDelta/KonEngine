#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>

class Node {
public:
    std::string name;
    bool active = true;

    Node(const std::string& name = "Node") : name(name) {}
    virtual ~Node() = default;

    virtual void Update(float dt) {}
    virtual void Draw() {}

    // Children
    template<typename T, typename... Args>
    T* AddChild(const std::string& childName, Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        node->name = childName;
        T* ptr = node.get();
        children.push_back(std::move(node));
        return ptr;
    }

    void RemoveChild(const std::string& childName) {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [&](const std::unique_ptr<Node>& n) {
                    return n->name == childName;
                }),
            children.end()
        );
    }

    Node* GetNode(const std::string& childName) {
        for (auto& child : children) {
            if (child->name == childName) return child.get();
            Node* found = child->GetNode(childName);
            if (found) return found;
        }
        return nullptr;
    }

    // Signals
    void Connect(const std::string& signal, std::function<void()> callback) {
        signals[signal].push_back(callback);
    }

    void Emit(const std::string& signal) {
        auto it = signals.find(signal);
        if (it != signals.end())
            for (auto& cb : it->second)
                cb();
    }

    // Update and draw all children
    void UpdateChildren(float dt) {
        for (auto& child : children)
            if (child->active) {
                child->Update(dt);
                child->UpdateChildren(dt);
            }
    }

    void DrawChildren() {
        for (auto& child : children)
            if (child->active) {
                child->Draw();
                child->DrawChildren();
            }
    }

protected:
    std::vector<std::unique_ptr<Node>> children;
    std::unordered_map<std::string, std::vector<std::function<void()>>> signals;
};
