#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <mutex>

// Forward declare so Node can have OnCollisionEnter/Exit
class Collider2D;
class CollisionWorld;

class Node {
public:
    std::string name;
    bool active = true;
    Node* parent = nullptr;

    // Set by Scene::Add — gives body nodes access to collision queries
    CollisionWorld* _world = nullptr;

    // Optional callback set by Scene so dynamically added collider children
    // get registered with the CollisionWorld automatically, no Scan() needed.
    std::function<void(Node*)> _onChildAdded;

    Node(const std::string& name = "Node") : name(name) {}
    virtual ~Node() = default;

    // Lifecycle -- override in subclasses
    virtual void Ready() {}
    virtual void Update(float dt) {}
    virtual void Draw() {}
    virtual void OnDestroy() {}  // called when node is removed from scene/parent

    // Collision callbacks -- override in nodes that have collider children
    // Called by Collider2D::Emit() when a collision signal fires on a child collider
    virtual void OnCollisionEnter(Collider2D* other) {}
    virtual void OnCollisionExit(Collider2D* other)  {}

    // Children
    template<typename T, typename... Args>
    T* AddChild(const std::string& childName, Args&&... args) {
        auto node = std::make_unique<T>(childName, std::forward<Args>(args)...);
        node->name   = childName;
        node->parent = this;
        // Propagate scene pointers down to children
        if (_onChildAdded) node->_onChildAdded = _onChildAdded;
        if (_world) node->_world = _world;
        T* ptr = node.get();
        {
            std::lock_guard<std::mutex> lock(m_childMutex);
            children.push_back(std::unique_ptr<Node>(std::move(node)));
        }
        ptr->Ready();
        // Notify scene so it can register any colliders
        if (_onChildAdded) _onChildAdded(static_cast<Node*>(ptr));
        return ptr;
    }

    void RemoveChild(const std::string& childName) {
        std::lock_guard<std::mutex> lock(m_childMutex);
        auto it = std::remove_if(children.begin(), children.end(),
            [&](const std::unique_ptr<Node>& n) { return n->name == childName; });
        // Call OnDestroy on removed nodes before erasing
        for (auto ri = it; ri != children.end(); ++ri) {
            (*ri)->OnDestroy();
        }
        children.erase(it, children.end());
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

    // Signals
    void Connect(const std::string& signal, std::function<void()> cb) {
        signals[signal].push_back(cb);
    }
    void Emit(const std::string& signal) {
        auto it = signals.find(signal);
        if (it != signals.end())
            for (auto& cb : it->second) cb();
    }

    virtual void UpdateChildren(float dt) {
        std::lock_guard<std::mutex> lock(m_childMutex);
        for (auto& child : children)
            if (child->active) {
                child->Update(dt);
                child->UpdateChildren(dt);
            }
    }

    virtual void DrawChildren() {
        std::lock_guard<std::mutex> lock(m_childMutex);
        for (auto& child : children)
            if (child->active) {
                child->Draw();
                child->DrawChildren();
            }
    }

    const std::vector<std::unique_ptr<Node>>& getChildren() const { return children; }

protected:
    mutable std::mutex m_childMutex;
    std::vector<std::unique_ptr<Node>> children;
    std::unordered_map<std::string, std::vector<std::function<void()>>> signals;
};
