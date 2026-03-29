#include "collision_world.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

void CollisionWorld::Add(Collider2D* collider) {
    colliders.push_back(collider);
}

void CollisionWorld::Remove(Collider2D* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider),
                    colliders.end());
    for (auto it = activePairs.begin(); it != activePairs.end(); ) {
        if (it->first == collider || it->second == collider)
            it = activePairs.erase(it);
        else
            ++it;
    }
}

void CollisionWorld::Clear() {
    colliders.clear();
    activePairs.clear();
}

void CollisionWorld::Update() {
    std::set<std::pair<Collider2D*, Collider2D*>> currentPairs;

    for (size_t i = 0; i < colliders.size(); i++) {
        Collider2D* a = colliders[i];
        if (!a->active) continue;

        for (size_t j = i + 1; j < colliders.size(); j++) {
            Collider2D* b = colliders[j];
            if (!b->active) continue;
            if (!LayersOverlap(a, b)) continue;

            if (Overlaps(a, b)) {
                auto pair = MakePair(a, b);
                currentPairs.insert(pair);
                if (activePairs.find(pair) == activePairs.end()) {
                    a->Emit("on_collision_enter", b);
                    b->Emit("on_collision_enter", a);
                }
            }
        }
    }

    // Fire exit signals for pairs that ended
    for (auto& pair : activePairs) {
        if (currentPairs.find(pair) == currentPairs.end()) {
            pair.first->Emit("on_collision_exit", pair.second);
            pair.second->Emit("on_collision_exit", pair.first);
        }
    }

    activePairs = currentPairs;

    // Update touching flag for debug visualization
    for (auto* col : colliders)
        col->touching = false;
    for (auto& pair : activePairs) {
        pair.first->touching  = true;
        pair.second->touching = true;
    }
}

bool CollisionWorld::Overlaps(Collider2D* a, Collider2D* b) {
    bool aCircle = a->shape == ColliderShape::Circle;
    bool bCircle = b->shape == ColliderShape::Circle;

    if (aCircle && bCircle)
        return SATCircleVsCircle(a, b);

    if (aCircle && !bCircle)
        return SATCircleVsPolygon({ a->worldX(), a->worldY() }, a->radius,
                                   b->GetWorldPoints());
    if (!aCircle && bCircle)
        return SATCircleVsPolygon({ b->worldX(), b->worldY() }, b->radius,
                                   a->GetWorldPoints());

    return SATPolygonVsPolygon(a->GetWorldPoints(), b->GetWorldPoints());
}

void CollisionWorld::ProjectOntoAxis(const std::vector<glm::vec2>& points,
                                      glm::vec2 axis, float& outMin, float& outMax) {
    outMin = outMax = glm::dot(points[0], axis);
    for (size_t i = 1; i < points.size(); i++) {
        float proj = glm::dot(points[i], axis);
        if (proj < outMin) outMin = proj;
        if (proj > outMax) outMax = proj;
    }
}

bool CollisionWorld::SATPolygonVsPolygon(const std::vector<glm::vec2>& a,
                                          const std::vector<glm::vec2>& b) {
    auto testAxes = [&](const std::vector<glm::vec2>& poly) -> bool {
        for (size_t i = 0; i < poly.size(); i++) {
            glm::vec2 edge = poly[(i+1) % poly.size()] - poly[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));
            float minA, maxA, minB, maxB;
            ProjectOntoAxis(a, axis, minA, maxA);
            ProjectOntoAxis(b, axis, minB, maxB);
            if (maxA < minB || maxB < minA) return false;
        }
        return true;
    };
    return testAxes(a) && testAxes(b);
}

bool CollisionWorld::SATCircleVsPolygon(glm::vec2 center, float radius,
                                         const std::vector<glm::vec2>& poly) {
    for (size_t i = 0; i < poly.size(); i++) {
        glm::vec2 edge = poly[(i+1) % poly.size()] - poly[i];
        glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));
        float minP, maxP;
        ProjectOntoAxis(poly, axis, minP, maxP);
        float proj = glm::dot(center, axis);
        if ((proj + radius) < minP || maxP < (proj - radius)) return false;
    }
    // Nearest vertex axis
    float minDist = std::numeric_limits<float>::max();
    glm::vec2 nearest = poly[0];
    for (auto& v : poly) {
        float d = glm::length(center - v);
        if (d < minDist) { minDist = d; nearest = v; }
    }
    glm::vec2 axis = glm::normalize(center - nearest);
    float minP, maxP;
    ProjectOntoAxis(poly, axis, minP, maxP);
    float proj = glm::dot(center, axis);
    if ((proj + radius) < minP || maxP < (proj - radius)) return false;
    return true;
}

bool CollisionWorld::SATCircleVsCircle(Collider2D* a, Collider2D* b) {
    float dx = a->worldX() - b->worldX();
    float dy = a->worldY() - b->worldY();
    float radSum = a->radius + b->radius;
    return (dx*dx + dy*dy) < (radSum * radSum);
}
