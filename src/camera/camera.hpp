#pragma once

struct Camera2D {
    float x, y;
    float zoom;
    float rotation;

    Camera2D(float x = 0, float y = 0, float zoom = 1.0f, float rotation = 0.0f)
        : x(x), y(y), zoom(zoom), rotation(rotation) {}
};
