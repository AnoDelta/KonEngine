#include "camera.hpp"
#include <cmath>
#include <cstdlib>

Camera2D Camera2DLerp(const Camera2D& from, const Camera2D& to, float t) {
    return Camera2D(
        from.x        + (to.x        - from.x)        * t,
        from.y        + (to.y        - from.y)        * t,
        from.zoom     + (to.zoom     - from.zoom)     * t,
        from.rotation + (to.rotation - from.rotation) * t
    );
}

void Camera2DFollow(Camera2D& cam, float targetX, float targetY,
                    float speed, float dt) {
    // Frame-rate independent lerp: same feel at any FPS.
    float t = 1.0f - std::pow(1.0f - speed, dt * 60.0f);
    cam.x += (targetX - cam.x) * t;
    cam.y += (targetY - cam.y) * t;
}

void Camera2DClamp(Camera2D& cam, float worldX, float worldY,
                   float worldW, float worldH,
                   float viewW,  float viewH) {
    float halfW = (viewW / cam.zoom) * 0.5f;
    float halfH = (viewH / cam.zoom) * 0.5f;

    float minX = worldX + halfW,  maxX = worldX + worldW - halfW;
    float minY = worldY + halfH,  maxY = worldY + worldH - halfH;

    if (maxX > minX) { if (cam.x < minX) cam.x = minX; if (cam.x > maxX) cam.x = maxX; }
    else             { cam.x = worldX + worldW * 0.5f; }

    if (maxY > minY) { if (cam.y < minY) cam.y = minY; if (cam.y > maxY) cam.y = maxY; }
    else             { cam.y = worldY + worldH * 0.5f; }
}

void Camera2DShake(Camera2D& cam, float magnitude) {
    auto rnd = [](float r) -> float {
        return (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * r - r;
    };
    cam.x += rnd(magnitude);
    cam.y += rnd(magnitude);
}
