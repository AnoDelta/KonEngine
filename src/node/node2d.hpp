#pragma once
#include "node.hpp"

class Node2D : public Node {
public:
    float x = 0, y = 0;
    float scaleX = 1, scaleY = 1;
    float rotation = 0;

    Node2D(const std::string& name = "Node2D") : Node(name) {}

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }
};
