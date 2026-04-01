#pragma once
#include "../node/collider2d.hpp"
#include <vector>
#include <set>
#include <utility>

// Minimum Translation Vector — result of a solid collision query.
// normal points FROM b TO a (push a in this direction to resolve).
// depth is how far the two shapes are overlapping.
struct MTV {
    bool      hit   = false;
    glm::vec2 normal = {0, 0};
    float     depth  = 0.0f;
};

class CollisionWorld {
public:
    void Add(Collider2D* collider);
    void Remove(Collider2D* collider);
    void Clear();

    // Run all checks + solid depenetration — call once per frame
    void Update();

    // Bool-only overlap test (no signals, no depenetration)
    static bool Overlaps(Collider2D* a, Collider2D* b);

    // Full MTV query (used internally and available for manual queries)
    static MTV GetMTV(Collider2D* a, Collider2D* b);

private:
    std::vector<Collider2D*> colliders;
    std::set<std::pair<Collider2D*, Collider2D*>> activePairs;

    // MTV-returning SAT helpers
    static MTV SATPolygonVsPolygon(const std::vector<glm::vec2>& a,
                                    const std::vector<glm::vec2>& b,
                                    glm::vec2 centerA, glm::vec2 centerB);
    static MTV SATCircleVsPolygon (glm::vec2 center, float radius,
                                    const std::vector<glm::vec2>& poly);
    static MTV SATCircleVsCircle  (Collider2D* a, Collider2D* b);

    static void ProjectOntoAxis(const std::vector<glm::vec2>& pts,
                                 glm::vec2 axis, float& mn, float& mx);

    // Apply depenetration to a pair of solid colliders
    static void Resolve(Collider2D* a, Collider2D* b, const MTV& mtv);

    static bool LayersOverlap(Collider2D* a, Collider2D* b) {
        return (a->layer & b->mask) || (b->layer & a->mask);
    }
    static std::pair<Collider2D*, Collider2D*> MakePair(Collider2D* a, Collider2D* b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    }
};
