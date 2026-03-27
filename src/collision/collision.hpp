#pragma once

struct Rectangle {
    float x, y, width, height;
    Rectangle(float x = 0, float y = 0, float width = 0, float height = 0)
        : x(x), y(y), width(width), height(height) {}
};

struct Circle {
    float x, y, radius;
    Circle(float x = 0, float y = 0, float radius = 0)
        : x(x), y(y), radius(radius) {}
};

bool CheckCollisionRecs(const Rectangle& a, const Rectangle& b);
bool CheckCollisionCircles(const Circle& a, const Circle& b);
bool CheckCollisionCircleRec(const Circle& c, const Rectangle& r);
