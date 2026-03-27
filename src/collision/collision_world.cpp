#include "collision_world.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------

void CollisionWorld::Add(Collider2D* collider) {
    colliders.push_back(collider);
}

void CollisionWorld::Remove(Collider2D* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider),
                    colliders.end());

    // Clean up any active pairs involving this collider
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

// -------------------------------------------------------------------------
// Per-frame update
// -------------------------------------------------------------------------

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
                    // New contact — fire enter
                    a->Emit("on_collision_enter", b);
                    b->Emit("on_collision_enter", a);
                }
                // Could fire "on_collision_stay" here if desired
            }
        }
    }

    // Check for exits — pairs that were active but aren't anymore
    for (auto& pair : activePairs) {
        if (currentPairs.find(pair) == currentPairs.end()) {
            pair.first->Emit("on_collision_exit", pair.second);
            pair.second->Emit("on_collision_exit", pair.first);
        }
    }

    activePairs = currentPairs;
}

// -------------------------------------------------------------------------
// Public static overlap test
// -------------------------------------------------------------------------

bool CollisionWorld::Overlaps(Collider2D* a, Collider2D* b) {
    bool aCircle = a->shape == ColliderShape::Circle;
    bool bCircle = b->shape == ColliderShape::Circle;

    if (aCircle && bCircle)
        return SATCircleVsCircle(a, b);

    if (aCircle && !bCircle)
        return SATCircleVsPolygon({ a->x, a->y }, a->radius, b->GetWorldPoints());

    if (!aCircle && bCircle)
        return SATCircleVsPolygon({ b->x, b->y }, b->radius, a->GetWorldPoints());

    return SATPolygonVsPolygon(a->GetWorldPoints(), b->GetWorldPoints());
}

// -------------------------------------------------------------------------
// SAT — Polygon vs Polygon
// -------------------------------------------------------------------------

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
            glm::vec2 edge = poly[(i + 1) % poly.size()] - poly[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float minA, maxA, minB, maxB;
            ProjectOntoAxis(a, axis, minA, maxA);
            ProjectOntoAxis(b, axis, minB, maxB);

            if (maxA < minB || maxB < minA)
                return false; // Separating axis found — no collision
        }
        return true;
    };

    return testAxes(a) && testAxes(b);
}

// -------------------------------------------------------------------------
// SAT — Circle vs Polygon
// -------------------------------------------------------------------------

bool CollisionWorld::SATCircleVsPolygon(glm::vec2 center, float radius,
                                         const std::vector<glm::vec2>& poly) {
    // Test edge normals of the polygon
    for (size_t i = 0; i < poly.size(); i++) {
        glm::vec2 edge = poly[(i + 1) % poly.size()] - poly[i];
        glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

        float minP, maxP;
        ProjectOntoAxis(poly, axis, minP, maxP);

        float proj = glm::dot(center, axis);
        float minC = proj - radius;
        float maxC = proj + radius;

        if (maxC < minP || maxP < minC)
            return false;
    }

    // Test axis from circle center to nearest polygon vertex
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
    float minC = proj - radius;
    float maxC = proj + radius;

    if (maxC < minP || maxP < minC)
        return false;

    return true;
}

// -------------------------------------------------------------------------
// Circle vs Circle
// -------------------------------------------------------------------------

bool CollisionWorld::SATCircleVsCircle(Collider2D* a, Collider2D* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float distSq = dx * dx + dy * dy;
    float radSum  = a->radius + b->radius;
    return distSq < radSum * radSum;
}
