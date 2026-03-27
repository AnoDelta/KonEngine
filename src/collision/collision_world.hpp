#pragma once
#include "../node/collider2d.hpp"
#include <vector>
#include <set>
#include <utility>

// SAT collision detection between two Collider2D nodes.
// CollisionWorld tracks enter/exit state and fires signals automatically.

class CollisionWorld {
public:
    // Register/unregister colliders manually (Scene does this automatically)
    void Add(Collider2D* collider);
    void Remove(Collider2D* collider);
    void Clear();

    // Run all checks — call once per frame (Scene::Update calls this)
    void Update();

    // Utility: test two specific colliders right now (no signal, just bool)
    static bool Overlaps(Collider2D* a, Collider2D* b);

private:
    std::vector<Collider2D*> colliders;

    // Pairs currently in contact — used to detect enter vs stay vs exit
    std::set<std::pair<Collider2D*, Collider2D*>> activePairs;

    // SAT helpers
    static bool SATPolygonVsPolygon(const std::vector<glm::vec2>& a,
                                     const std::vector<glm::vec2>& b);
    static bool SATCircleVsPolygon(glm::vec2 center, float radius,
                                    const std::vector<glm::vec2>& poly);
    static bool SATCircleVsCircle(Collider2D* a, Collider2D* b);

    static void ProjectOntoAxis(const std::vector<glm::vec2>& points,
                                 glm::vec2 axis, float& outMin, float& outMax);

    static bool LayersOverlap(Collider2D* a, Collider2D* b) {
        return (a->layer & b->mask) || (b->layer & a->mask);
    }

    static std::pair<Collider2D*, Collider2D*> MakePair(Collider2D* a, Collider2D* b) {
        // Canonical ordering so (a,b) == (b,a)
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    }
};
