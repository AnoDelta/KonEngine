#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>

// Forward declaration so OnCollisionEnter/Exit can reference Collider2D
class Collider2D;

class Node {
public:
    std::string name;
    bool        active = true;
    Node*       parent = nullptr;

    Node(const std::string& name = "Node") : name(name) {}
    virtual ~Node() = default;

    virtual void Ready() {}
    virtual void Update(float dt) {}
    virtual void Draw() {}
    virtual void OnCollisionEnter(Collider2D* other) {}
    virtual void OnCollisionExit(Collider2D* other) {}

    // Children
    template<typename T, typename... Args>
    T* AddChild(const std::string& childName, Args&&... args) {
        auto node = std::make_unique<T>(childName, std::forward<Args>(args)...);
        node->name   = childName;
        node->parent = this;
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

    void ForEachDescendant(std::function<void(Node*)> fn) {
        for (auto& child : children) {
            fn(child.get());
            child->ForEachDescendant(fn);
        }
    }

    Node* GetNode(const std::string& childName) {
        for (auto& child : children) {
            if (child->name == childName) return child.get();
            Node* found = child->GetNode(childName);
            if (found) return found;
        }
        return nullptr;
    }

    // Signals (no-arg)
    void Connect(const std::string& signal, std::function<void()> callback) {
        signals[signal].push_back(callback);
    }

    void Emit(const std::string& signal) {
        auto it = signals.find(signal);
        if (it != signals.end())
            for (auto& cb : it->second)
                cb();
    }

    virtual void UpdateChildren(float dt) {
        for (auto& child : children)
            if (child->active) {
                child->Update(dt);
                child->UpdateChildren(dt);
            }
    }

    virtual void DrawChildren() {
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
