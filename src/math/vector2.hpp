#pragma once

#include <glm/glm.hpp>
#include <cmath>

struct Vector2 {
    float x, y;

    Vector2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    // Conversion to/from glm::vec2
    Vector2(const glm::vec2& v) : x(v.x), y(v.y) {}
    operator glm::vec2() const { return glm::vec2(x, y); }

    // Arithmetic operators
    Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
    Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
    Vector2 operator*(float s)          const { return { x * s,   y * s   }; }
    Vector2 operator/(float s)          const { return { x / s,   y / s   }; }
    Vector2 operator-()                 const { return { -x, -y }; }

    Vector2& operator+=(const Vector2& o) { x += o.x; y += o.y; return *this; }
    Vector2& operator-=(const Vector2& o) { x -= o.x; y -= o.y; return *this; }
    Vector2& operator*=(float s)          { x *= s;   y *= s;   return *this; }
    Vector2& operator/=(float s)          { x /= s;   y /= s;   return *this; }

    bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vector2& o) const { return !(*this == o); }

    // Math
    float LengthSq() const { return x * x + y * y; }
    float Length()   const { return std::sqrt(LengthSq()); }

    Vector2 Normalized() const {
        float len = Length();
        if (len == 0.0f) return { 0.0f, 0.0f };
        return { x / len, y / len };
    }

    float Dot(const Vector2& o)      const { return x * o.x + y * o.y; }
    float Distance(const Vector2& o) const { return (*this - o).Length(); }
    float DistanceSq(const Vector2& o) const { return (*this - o).LengthSq(); }

    // Rotate by angle in radians
    Vector2 Rotated(float angle) const {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return { x * c - y * s, x * s + y * c };
    }

    // Reflect across a normal
    Vector2 Reflected(const Vector2& normal) const {
        return *this - normal * (2.0f * Dot(normal));
    }

    // Linear interpolation
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
        return a + (b - a) * t;
    }

    // Presets
    static Vector2 Zero()  { return { 0.0f,  0.0f }; }
    static Vector2 One()   { return { 1.0f,  1.0f }; }
    static Vector2 Up()    { return { 0.0f, -1.0f }; }
    static Vector2 Down()  { return { 0.0f,  1.0f }; }
    static Vector2 Left()  { return {-1.0f,  0.0f }; }
    static Vector2 Right() { return { 1.0f,  0.0f }; }
};

// Allow float * Vector2 as well as Vector2 * float
inline Vector2 operator*(float s, const Vector2& v) { return v * s; }
