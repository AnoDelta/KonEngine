#pragma once
#include "node2d.hpp"
#include "../camera/camera.hpp"
#include "../window/window.hpp"

// CameraNode2D — a node that acts as the scene camera.
// Add one to your scene and Scene::Draw() will automatically
// use it for BeginCamera2D/EndCamera2D.
class CameraNode2D : public Node2D {
public:
    float zoom     = 1.0f;
    float rotation = 0.0f;
    bool  current  = true;  // if true, this camera is active

    CameraNode2D(const std::string& name = "CameraNode2D") : Node2D(name) {}

    // Build a Camera2D struct from this node's world position
    Camera2D toCamera2D() const {
        return Camera2D{ x, y, zoom, rotation };
    }

    void Draw() override {
        // Don't draw children here — Scene::Draw handles camera wrapping
        Node2D::DrawChildren();
    }
};
