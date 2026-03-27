#include "collision.hpp"
#include <cmath>

bool CheckCollisionRecs(const Rectangle& a, const Rectangle& b) {
    return a.x < b.x + b.width  &&
           a.x + a.width > b.x  &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

bool CheckCollisionCircles(const Circle& a, const Circle& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < a.radius + b.radius;
}

bool CheckCollisionCircleRec(const Circle& c, const Rectangle& r) {
    float nearestX = std::fmax(r.x, std::fmin(c.x, r.x + r.width));
    float nearestY = std::fmax(r.y, std::fmin(c.y, r.y + r.height));
    float dx = c.x - nearestX;
    float dy = c.y - nearestY;
    return (dx * dx + dy * dy) < (c.radius * c.radius);
}
