#!/bin/bash
# fix-cross-compile.sh
# Run from the root of your KonEngine repo:
#   bash fix-cross-compile.sh
set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "==> Applying cross-compilation fixes in: $REPO_ROOT"

# -----------------------------------------------------------------------
# 1. src/camera/camera.hpp  — add utility function declarations
# -----------------------------------------------------------------------
cat > "$REPO_ROOT/src/camera/camera.hpp" << 'EOF'
#pragma once

struct Camera2D {
    float x, y;
    float zoom;
    float rotation;

    Camera2D(float x = 0, float y = 0, float zoom = 1.0f, float rotation = 0.0f)
        : x(x), y(y), zoom(zoom), rotation(rotation) {}
};

// ---------------------------------------------------------------------------
// Utility free functions (implemented in camera.cpp)
// ---------------------------------------------------------------------------

// Linearly interpolate between two cameras (t = 0..1).
Camera2D Camera2DLerp(const Camera2D& from, const Camera2D& to, float t);

// Smooth-follow: move cam toward (targetX, targetY) each frame.
// speed ~0.1 (slow) .. ~0.9 (fast). Pass GetDeltaTime() as dt.
void Camera2DFollow(Camera2D& cam, float targetX, float targetY,
                    float speed, float dt);

// Clamp cam so it never reveals outside the world rectangle.
// worldX/Y = top-left of world, worldW/H = world size.
// viewW/H  = viewport size (use GetWindowWidth / GetWindowHeight).
void Camera2DClamp(Camera2D& cam, float worldX, float worldY,
                   float worldW, float worldH,
                   float viewW,  float viewH);

// Offset cam by a random amount within [-magnitude, magnitude].
// Decay magnitude each frame (e.g. magnitude *= 0.9f) for a shake effect.
void Camera2DShake(Camera2D& cam, float magnitude);
EOF
echo "    [OK] src/camera/camera.hpp"

# -----------------------------------------------------------------------
# 2. src/camera/camera.cpp  — implement the utility functions
# -----------------------------------------------------------------------
cat > "$REPO_ROOT/src/camera/camera.cpp" << 'EOF'
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
EOF
echo "    [OK] src/camera/camera.cpp"

# -----------------------------------------------------------------------
# 3. CMakeLists.txt  — add src/camera/*.cpp to GLOB_RECURSE
# -----------------------------------------------------------------------
CMAKE="$REPO_ROOT/CMakeLists.txt"

if grep -q "src/camera/\*\.cpp" "$CMAKE"; then
    echo "    [SKIP] CMakeLists.txt already has src/camera/*.cpp"
else
    # Insert after the src/collision/*.cpp line
    sed -i 's|        src/collision/\*\.cpp|        src/camera/*.cpp\n        src/collision/*.cpp|' "$CMAKE"
    echo "    [OK] CMakeLists.txt — added src/camera/*.cpp to GLOB_RECURSE"
fi

# -----------------------------------------------------------------------
# 4. src/window/window.cpp  — fix GLFW_PLATFORM_X11 cross-compile guard
# -----------------------------------------------------------------------
WINDOW="$REPO_ROOT/src/window/window.cpp"

# Replace the old single-condition ifdef with a Windows-safe version.
# The old line is:  #ifdef GLFW_PLATFORM_X11
# We change it to: #if defined(GLFW_PLATFORM_X11) && !defined(_WIN32)
if grep -q "defined(GLFW_PLATFORM_X11) && !defined(_WIN32)" "$WINDOW"; then
    echo "    [SKIP] window.cpp already patched"
else
    sed -i 's|#ifdef GLFW_PLATFORM_X11|#if defined(GLFW_PLATFORM_X11) \&\& !defined(_WIN32)|' "$WINDOW"
    echo "    [OK] src/window/window.cpp — GLFW_PLATFORM_X11 guard is now Windows-safe"
fi

# -----------------------------------------------------------------------
echo ""
echo "==> All fixes applied. Rebuild:"
echo "      rm -rf build && cmake -B build && cmake --build build"
echo ""
