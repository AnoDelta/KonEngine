#include "collision_world.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

// ── Registration ──────────────────────────────────────────────────────────

void CollisionWorld::Add(Collider2D* collider) {
    std::lock_guard<std::mutex> lock(m_mutex);
    colliders.push_back(collider);
}

void CollisionWorld::Remove(Collider2D* collider) {
    std::lock_guard<std::mutex> lock(m_mutex);
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider),
                    colliders.end());
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
    std::lock_guard<std::mutex> lock(m_mutex);
    colliders.clear();
    activePairs.clear();
}

// ── Main update ───────────────────────────────────────────────────────────

void CollisionWorld::Update() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::set<std::pair<Collider2D*, Collider2D*>> currentPairs;

    for (size_t i = 0; i < colliders.size(); i++) {
        Collider2D* a = colliders[i];
        if (!a->active) continue;

        for (size_t j = i + 1; j < colliders.size(); j++) {
            Collider2D* b = colliders[j];
            if (!b->active) continue;
            if (!LayersOverlap(a, b)) continue;

            // Broad-phase: cheap AABB rejection before expensive SAT
            if (!AABBOverlap(a, b)) continue;

            MTV mtv = GetMTV(a, b);
            if (!mtv.hit) continue;

            auto pair = MakePair(a, b);
            currentPairs.insert(pair);

            // Fire enter signal if this is a new contact
            if (activePairs.find(pair) == activePairs.end()) {
                a->Emit("on_collision_enter", b);
                b->Emit("on_collision_enter", a);
            }

            // Depenetrate solid colliders
            if (a->solid && b->solid)
                Resolve(a, b, mtv);
        }
    }

    // Fire exit signals for pairs that are no longer touching
    for (auto& pair : activePairs) {
        if (currentPairs.find(pair) == currentPairs.end()) {
            pair.first->Emit("on_collision_exit",  pair.second);
            pair.second->Emit("on_collision_exit", pair.first);
        }
    }
    activePairs = currentPairs;

    // Rebuild contact lists
    for (auto* c : colliders) c->contacts.clear();
    for (auto& pair : activePairs) {
        pair.first->contacts.push_back(pair.second);
        pair.second->contacts.push_back(pair.first);
    }
}

// ── Depenetration ─────────────────────────────────────────────────────────
//
// MTV.normal points from b toward a — so pushing a by +normal and b by -normal
// separates them.
//
// Distribution:
//   both dynamic  → each moves half
//   a static only → push b the full amount
//   b static only → push a the full amount
//   both static   → do nothing (shouldn't collide, but be safe)

// Helper: find the body node that owns this collider
static Node2D* FindBody(Collider2D* col) {
    Node* p = col->parent;
    while (p) {
        if (auto* body = dynamic_cast<Node2D*>(p))
            if (!dynamic_cast<Collider2D*>(body))
                return body;
        p = p->parent;
    }
    return nullptr;
}

static void PushNode(Collider2D* col, float px, float py) {
    Node2D* body = FindBody(col);
    if (body) { body->x += px; body->y += py; }
    else      { col->x += px; col->y += py; }
}

void CollisionWorld::Resolve(Collider2D* a, Collider2D* b, const MTV& mtv) {
    if (a->staticBody && b->staticBody) return;

    float d = mtv.depth;
    if (d <= 0.0f) return;

    glm::vec2 push = mtv.normal * d;

    if (!a->staticBody && !b->staticBody) {
        PushNode(a,  push.x * 0.5f,  push.y * 0.5f);
        PushNode(b, -push.x * 0.5f, -push.y * 0.5f);
    } else if (a->staticBody) {
        PushNode(b, -push.x, -push.y);
    } else {
        PushNode(a,  push.x,  push.y);
    }
}

// ── Public query helpers ──────────────────────────────────────────────────

glm::vec2 CollisionWorld::ResolveOverlap(Collider2D* mover, Collider2D* wall) {
    MTV mtv = GetMTV(mover, wall);
    if (!mtv.hit || mtv.depth <= 0.0f)
        return {0.0f, 0.0f};

    // mtv.normal points from wall toward mover, so pushing mover along
    // +normal separates them.
    constexpr float slop = 0.0f;
    float d = std::max(mtv.depth - slop, 0.0f);
    if (d == 0.0f) return {0.0f, 0.0f};

    glm::vec2 push = mtv.normal * d;
    mover->x += push.x;
    mover->y += push.y;
    return push;
}

// Global debug flag — set via CollisionDebug(true) from game code
bool s_collisionDebug = false;
void CollisionDebug(bool enabled) { s_collisionDebug = enabled; }

glm::vec2 CollisionWorld::SweepResolve(Collider2D* mover) {
    glm::vec2 totalPush(0.0f, 0.0f);

    float origX = mover->x, origY = mover->y;

    // Compute mover's world position for debug output
    auto moverWorld = mover->computeWorldPivot();

    if (s_collisionDebug) {
        fprintf(stderr, "[Collision] SweepResolve '%s': local(%.1f,%.1f) world(%.1f,%.1f) w=%.0f h=%.0f origin(%.1f,%.1f)\n",
                mover->name.c_str(), mover->x, mover->y,
                moverWorld.x, moverWorld.y,
                mover->width, mover->height,
                mover->originX, mover->originY);
    }

    int staticCount = 0;
    for (int iter = 0; iter < 4; ++iter) {
        bool pushed = false;
        for (auto* other : colliders) {
            if (other == mover || !other->active || !other->staticBody) continue;
            if (!LayersOverlap(mover, other)) continue;

            if (iter == 0 && s_collisionDebug) {
                auto otherWorld = other->computeWorldPivot();
                auto otherTL = other->computeWorldTopLeft();
                staticCount++;
                fprintf(stderr, "  vs static '%s': world(%.1f,%.1f) tl(%.1f,%.1f) w=%.0f h=%.0f origin(%.1f,%.1f)\n",
                        other->name.c_str(),
                        otherWorld.x, otherWorld.y,
                        otherTL.x, otherTL.y,
                        other->width, other->height,
                        other->originX, other->originY);
            }

            if (!AABBOverlap(mover, other)) {
                if (iter == 0 && s_collisionDebug) fprintf(stderr, "    AABB: no overlap\n");
                continue;
            }

            MTV mtv = GetMTV(mover, other);
            if (!mtv.hit || mtv.depth <= 0.0f) {
                if (iter == 0 && s_collisionDebug) fprintf(stderr, "    MTV: no hit (depth=%.3f)\n", mtv.depth);
                continue;
            }

            constexpr float slop = 0.0f;
            float d = std::max(mtv.depth - slop, 0.0f);
            if (d == 0.0f) continue;

            glm::vec2 push = mtv.normal * d;
            mover->x += push.x;
            mover->y += push.y;
            totalPush += push;
            pushed = true;

            if (s_collisionDebug) {
                fprintf(stderr, "    HIT iter=%d: depth=%.3f normal=(%.2f,%.2f) push=(%.2f,%.2f)\n",
                        iter, mtv.depth, mtv.normal.x, mtv.normal.y, push.x, push.y);
            }
        }
        if (!pushed) break;
    }

    if (s_collisionDebug && staticCount == 0) {
        fprintf(stderr, "  WARNING: no static colliders found in world! (%zu total colliders)\n", colliders.size());
        for (auto* c : colliders) {
            fprintf(stderr, "    collider '%s': solid=%d static=%d active=%d\n",
                    c->name.c_str(), c->solid, c->staticBody, c->active);
        }
    }

    mover->x = origX;
    mover->y = origY;

    if (s_collisionDebug && (totalPush.x != 0 || totalPush.y != 0)) {
        fprintf(stderr, "  TOTAL PUSH: (%.2f, %.2f)\n", totalPush.x, totalPush.y);
    }

    return totalPush;
}

bool CollisionWorld::Overlaps(Collider2D* a, Collider2D* b) {
    return GetMTV(a, b).hit;
}

MTV CollisionWorld::GetMTV(Collider2D* a, Collider2D* b) {
    bool aCircle = a->shape == ColliderShape::Circle;
    bool bCircle = b->shape == ColliderShape::Circle;

    if (aCircle && bCircle)
        return SATCircleVsCircle(a, b);

    if (aCircle && !bCircle)
        return SATCircleVsPolygon(a->worldCenter(), a->radius, b->GetWorldPoints());

    if (!aCircle && bCircle) {
        // Flip result so normal still points a→b correctly
        MTV m = SATCircleVsPolygon(b->worldCenter(), b->radius, a->GetWorldPoints());
        m.normal = -m.normal;
        return m;
    }

    return SATPolygonVsPolygon(a->GetWorldPoints(), b->GetWorldPoints(),
                                a->worldCenter(),    b->worldCenter());
}

// ── Broad-phase AABB ──────────────────────────────────────────────────────

bool CollisionWorld::AABBOverlap(Collider2D* a, Collider2D* b) {
    // For circles: use center +/- radius as bounding box
    // For rectangles/polygons: use the world top-left + size
    float ax, ay, aw, ah;
    float bx, by, bw, bh;

    if (a->shape == ColliderShape::Circle) {
        auto c = a->worldCenter();
        ax = c.x - a->radius; ay = c.y - a->radius;
        aw = a->radius * 2; ah = a->radius * 2;
    } else {
        auto tl = a->computeWorldTopLeft();
        float wsx = std::fabs(a->scaleX), wsy = std::fabs(a->scaleY);
        Node* p = a->parent;
        while (p) {
            auto* p2d = dynamic_cast<Node2D*>(p);
            if (!p2d) break;
            wsx *= std::fabs(p2d->scaleX);
            wsy *= std::fabs(p2d->scaleY);
            p = p2d->parent;
        }
        ax = tl.x; ay = tl.y;
        aw = a->width * wsx; ah = a->height * wsy;
    }

    if (b->shape == ColliderShape::Circle) {
        auto c = b->worldCenter();
        bx = c.x - b->radius; by = c.y - b->radius;
        bw = b->radius * 2; bh = b->radius * 2;
    } else {
        auto tl = b->computeWorldTopLeft();
        float wsx = std::fabs(b->scaleX), wsy = std::fabs(b->scaleY);
        Node* p = b->parent;
        while (p) {
            auto* p2d = dynamic_cast<Node2D*>(p);
            if (!p2d) break;
            wsx *= std::fabs(p2d->scaleX);
            wsy *= std::fabs(p2d->scaleY);
            p = p2d->parent;
        }
        bx = tl.x; by = tl.y;
        bw = b->width * wsx; bh = b->height * wsy;
    }

    return !(ax + aw < bx || bx + bw < ax || ay + ah < by || by + bh < ay);
}

// ── SAT helpers ───────────────────────────────────────────────────────────

void CollisionWorld::ProjectOntoAxis(const std::vector<glm::vec2>& pts,
                                      glm::vec2 axis, float& mn, float& mx) {
    mn = mx = glm::dot(pts[0], axis);
    for (size_t i = 1; i < pts.size(); i++) {
        float p = glm::dot(pts[i], axis);
        if (p < mn) mn = p;
        if (p > mx) mx = p;
    }
}

// Polygon vs Polygon — returns MTV where normal points from b toward a.
MTV CollisionWorld::SATPolygonVsPolygon(const std::vector<glm::vec2>& a,
                                         const std::vector<glm::vec2>& b,
                                         glm::vec2 centerA, glm::vec2 centerB) {
    MTV result;
    result.hit   = true;
    result.depth = std::numeric_limits<float>::max();

    auto testAxes = [&](const std::vector<glm::vec2>& poly) -> bool {
        for (size_t i = 0; i < poly.size(); i++) {
            glm::vec2 edge = poly[(i+1) % poly.size()] - poly[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float mnA, mxA, mnB, mxB;
            ProjectOntoAxis(a, axis, mnA, mxA);
            ProjectOntoAxis(b, axis, mnB, mxB);

            // Separating axis found — no collision
            if (mxA < mnB || mxB < mnA) return false;

            // Overlap on this axis
            float overlap = std::min(mxA, mxB) - std::max(mnA, mnB);
            if (overlap < result.depth) {
                result.depth  = overlap;
                result.normal = axis;
            }
        }
        return true;
    };

    if (!testAxes(a)) { result.hit = false; return result; }
    if (!testAxes(b)) { result.hit = false; return result; }

    // Make sure normal points from b toward a
    glm::vec2 dir = centerA - centerB;
    if (glm::dot(result.normal, dir) < 0.0f)
        result.normal = -result.normal;

    return result;
}

// Circle vs Polygon
MTV CollisionWorld::SATCircleVsPolygon(glm::vec2 center, float radius,
                                        const std::vector<glm::vec2>& poly) {
    MTV result;
    result.hit   = true;
    result.depth = std::numeric_limits<float>::max();

    // Test polygon edge normals
    for (size_t i = 0; i < poly.size(); i++) {
        glm::vec2 edge = poly[(i+1) % poly.size()] - poly[i];
        glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

        float mn, mx;
        ProjectOntoAxis(poly, axis, mn, mx);
        float proj = glm::dot(center, axis);

        if ((proj + radius) < mn || mx < (proj - radius)) {
            result.hit = false;
            return result;
        }

        float overlap = std::min(mx, proj + radius) - std::max(mn, proj - radius);
        if (overlap < result.depth) {
            result.depth  = overlap;
            result.normal = axis;
        }
    }

    // Test axis from nearest vertex to circle center
    float best = std::numeric_limits<float>::max();
    glm::vec2 nearest = poly[0];
    for (auto& v : poly) {
        float d = glm::length(center - v);
        if (d < best) { best = d; nearest = v; }
    }
    glm::vec2 axis = glm::normalize(center - nearest);
    float mn, mx;
    ProjectOntoAxis(poly, axis, mn, mx);
    float proj = glm::dot(center, axis);

    if ((proj + radius) < mn || mx < (proj - radius)) {
        result.hit = false;
        return result;
    }

    float overlap = std::min(mx, proj + radius) - std::max(mn, proj - radius);
    if (overlap < result.depth) {
        result.depth  = overlap;
        result.normal = axis;
    }

    // Normal should point from polygon center toward circle
    glm::vec2 polyCen = {0, 0};
    for (auto& v : poly) polyCen += v;
    polyCen /= (float)poly.size();

    if (glm::dot(result.normal, center - polyCen) < 0.0f)
        result.normal = -result.normal;

    return result;
}

// Circle vs Circle
MTV CollisionWorld::SATCircleVsCircle(Collider2D* a, Collider2D* b) {
    MTV result;
    glm::vec2 ac = a->worldCenter(), bc = b->worldCenter();
    glm::vec2 delta = ac - bc;
    float distSq = delta.x*delta.x + delta.y*delta.y;
    float r      = a->radius + b->radius;

    if (distSq >= r * r) {
        result.hit = false;
        return result;
    }

    float dist = std::sqrt(distSq);
    result.hit    = true;
    result.depth  = r - dist;
    result.normal = (dist > 0.0001f)
        ? glm::normalize(delta)
        : glm::vec2{1.0f, 0.0f}; // degenerate case: same position

    return result;
}
