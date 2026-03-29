#include "collision_world.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

void CollisionWorld::Add(Collider2D* c)    { colliders.push_back(c); }
void CollisionWorld::Remove(Collider2D* c) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), c), colliders.end());
    for (auto it = activePairs.begin(); it != activePairs.end(); )
        (it->first == c || it->second == c) ? it = activePairs.erase(it) : ++it;
}
void CollisionWorld::Clear() { colliders.clear(); activePairs.clear(); }

void CollisionWorld::Update() {
    std::set<std::pair<Collider2D*, Collider2D*>> currentPairs;

    for (size_t i = 0; i < colliders.size(); i++) {
        Collider2D* a = colliders[i];
        if (!a->active) continue;
        for (size_t j = i+1; j < colliders.size(); j++) {
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

    for (auto& pair : activePairs)
        if (currentPairs.find(pair) == currentPairs.end()) {
            pair.first->Emit("on_collision_exit",  pair.second);
            pair.second->Emit("on_collision_exit", pair.first);
        }

    activePairs = currentPairs;

    // Update touching flag for debug highlight
    for (auto* c : colliders) c->touching = false;
    for (auto& p : activePairs) { p.first->touching = true; p.second->touching = true; }
}

bool CollisionWorld::Overlaps(Collider2D* a, Collider2D* b) {
    bool aCirc = a->shape == ColliderShape::Circle;
    bool bCirc = b->shape == ColliderShape::Circle;

    if (aCirc && bCirc) return SATCircleVsCircle(a, b);

    auto aPts = a->GetWorldPoints();
    auto bPts = b->GetWorldPoints();

    if (aCirc) return SATCircleVsPolygon(a->worldCenter(), a->radius, bPts);
    if (bCirc) return SATCircleVsPolygon(b->worldCenter(), b->radius, aPts);
    return SATPolygonVsPolygon(aPts, bPts);
}

void CollisionWorld::ProjectOntoAxis(const std::vector<glm::vec2>& pts,
                                      glm::vec2 axis, float& mn, float& mx) {
    mn = mx = glm::dot(pts[0], axis);
    for (size_t i = 1; i < pts.size(); i++) {
        float p = glm::dot(pts[i], axis);
        if (p < mn) mn = p;
        if (p > mx) mx = p;
    }
}

bool CollisionWorld::SATPolygonVsPolygon(const std::vector<glm::vec2>& a,
                                          const std::vector<glm::vec2>& b) {
    auto test = [&](const std::vector<glm::vec2>& poly) {
        for (size_t i = 0; i < poly.size(); i++) {
            glm::vec2 edge = poly[(i+1)%poly.size()] - poly[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));
            float mnA, mxA, mnB, mxB;
            ProjectOntoAxis(a, axis, mnA, mxA);
            ProjectOntoAxis(b, axis, mnB, mxB);
            if (mxA < mnB || mxB < mnA) return false;
        }
        return true;
    };
    return test(a) && test(b);
}

bool CollisionWorld::SATCircleVsPolygon(glm::vec2 center, float radius,
                                         const std::vector<glm::vec2>& poly) {
    for (size_t i = 0; i < poly.size(); i++) {
        glm::vec2 edge = poly[(i+1)%poly.size()] - poly[i];
        glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));
        float mn, mx;
        ProjectOntoAxis(poly, axis, mn, mx);
        float proj = glm::dot(center, axis);
        if ((proj+radius) < mn || mx < (proj-radius)) return false;
    }
    // Nearest vertex axis
    float best = std::numeric_limits<float>::max();
    glm::vec2 nearest = poly[0];
    for (auto& v : poly) { float d = glm::length(center-v); if (d < best) { best=d; nearest=v; } }
    glm::vec2 axis = glm::normalize(center - nearest);
    float mn, mx;
    ProjectOntoAxis(poly, axis, mn, mx);
    float proj = glm::dot(center, axis);
    if ((proj+radius) < mn || mx < (proj-radius)) return false;
    return true;
}

bool CollisionWorld::SATCircleVsCircle(Collider2D* a, Collider2D* b) {
    auto ac = a->worldCenter(), bc = b->worldCenter();
    float dx = ac.x - bc.x, dy = ac.y - bc.y;
    float r  = a->radius + b->radius;
    return (dx*dx + dy*dy) < (r*r);
}
