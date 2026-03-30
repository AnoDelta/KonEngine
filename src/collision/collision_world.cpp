#include "collision_world.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

void CollisionWorld::Add(Collider2D* collider) {
    colliders.push_back(collider);
}

void CollisionWorld::Remove(Collider2D* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
    for (auto it = activePairs.begin(); it != activePairs.end(); ) {
        if (it->first == collider || it->second == collider) {
            it->first->Emit("on_collision_exit",  it->second);
            it->second->Emit("on_collision_exit", it->first);
            auto& fc = it->first->contacts;
            fc.erase(std::remove(fc.begin(), fc.end(), it->second), fc.end());
            auto& sc = it->second->contacts;
            sc.erase(std::remove(sc.begin(), sc.end(), it->first), sc.end());
            it = activePairs.erase(it);
        } else { ++it; }
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
    for (auto& pair : activePairs) {
        if (currentPairs.find(pair) == currentPairs.end()) {
            pair.first->Emit("on_collision_exit", pair.second);
            pair.second->Emit("on_collision_exit", pair.first);
        }
    }
    activePairs = currentPairs;
    for (auto* c : colliders) c->contacts.clear();
    for (auto& pair : activePairs) {
        pair.first->contacts.push_back(pair.second);
        pair.second->contacts.push_back(pair.first);
    }
}

bool CollisionWorld::Overlaps(Collider2D* a, Collider2D* b) {
    bool aCircle = a->shape == ColliderShape::Circle;
    bool bCircle = b->shape == ColliderShape::Circle;
    if (aCircle && bCircle)  return SATCircleVsCircle(a, b);
    if (aCircle && !bCircle) return SATCircleVsPolygon({a->x,a->y}, a->radius, b->GetWorldPoints());
    if (!aCircle && bCircle) return SATCircleVsPolygon({b->x,b->y}, b->radius, a->GetWorldPoints());
    return SATPolygonVsPolygon(a->GetWorldPoints(), b->GetWorldPoints());
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
