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
