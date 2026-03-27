#pragma once
#include <cmath>

// All curves take t in [0, 1] and return a reshaped t in [0, 1]
// (Bounce and Elastic may slightly exceed [0,1] intentionally)

enum class Ease {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
};

namespace Curves {

    inline float Linear(float t) { return t; }

    // --- Quadratic ---
    inline float EaseIn(float t)    { return t * t; }
    inline float EaseOut(float t)   { return t * (2.0f - t); }
    inline float EaseInOut(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    // --- Cubic ---
    inline float EaseInCubic(float t)    { return t * t * t; }
    inline float EaseOutCubic(float t)   { float u = 1.0f - t; return 1.0f - u * u * u; }
    inline float EaseInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // --- Elastic ---
    inline float EaseInElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        return -std::pow(2.0f, 10.0f * t - 10.0f)
             * std::sin((t * 10.0f - 10.75f) * (2.0f * 3.14159265f) / 3.0f);
    }
    inline float EaseOutElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        return std::pow(2.0f, -10.0f * t)
             * std::sin((t * 10.0f - 0.75f) * (2.0f * 3.14159265f) / 3.0f) + 1.0f;
    }
    inline float EaseInOutElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        const float c = (2.0f * 3.14159265f) / 4.5f;
        return t < 0.5f
            ? -(std::pow(2.0f,  20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c)) / 2.0f
            :  (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c)) / 2.0f + 1.0f;
    }

    // --- Bounce ---
    inline float EaseOutBounce(float t) {
        const float n = 7.5625f, d = 2.75f;
        if (t < 1.0f / d)       return n * t * t;
        if (t < 2.0f / d)       { t -= 1.5f   / d; return n * t * t + 0.75f;    }
        if (t < 2.5f / d)       { t -= 2.25f  / d; return n * t * t + 0.9375f;  }
                                  t -= 2.625f / d;  return n * t * t + 0.984375f;
    }
    inline float EaseInBounce(float t)    { return 1.0f - EaseOutBounce(1.0f - t); }
    inline float EaseInOutBounce(float t) {
        return t < 0.5f
            ? (1.0f - EaseOutBounce(1.0f - 2.0f * t)) / 2.0f
            : (1.0f + EaseOutBounce(2.0f * t - 1.0f)) / 2.0f;
    }

    // --- Back (slight overshoot) ---
    inline float EaseInBack(float t) {
        const float c = 1.70158f;
        return (c + 1.0f) * t * t * t - c * t * t;
    }
    inline float EaseOutBack(float t) {
        const float c = 1.70158f;
        float u = t - 1.0f;
        return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
    }
    inline float EaseInOutBack(float t) {
        const float c = 1.70158f * 1.525f;
        return t < 0.5f
            ? (std::pow(2.0f * t, 2.0f) * ((c + 1.0f) * 2.0f * t - c)) / 2.0f
            : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c + 1.0f) * (2.0f * t - 2.0f) + c) + 2.0f) / 2.0f;
    }

    // --- Dispatch ---
    inline float Apply(Ease ease, float t) {
        switch (ease) {
            case Ease::Linear:           return Linear(t);
            case Ease::EaseIn:           return EaseIn(t);
            case Ease::EaseOut:          return EaseOut(t);
            case Ease::EaseInOut:        return EaseInOut(t);
            case Ease::EaseInCubic:      return EaseInCubic(t);
            case Ease::EaseOutCubic:     return EaseOutCubic(t);
            case Ease::EaseInOutCubic:   return EaseInOutCubic(t);
            case Ease::EaseInElastic:    return EaseInElastic(t);
            case Ease::EaseOutElastic:   return EaseOutElastic(t);
            case Ease::EaseInOutElastic: return EaseInOutElastic(t);
            case Ease::EaseInBounce:     return EaseInBounce(t);
            case Ease::EaseOutBounce:    return EaseOutBounce(t);
            case Ease::EaseInOutBounce:  return EaseInOutBounce(t);
            case Ease::EaseInBack:       return EaseInBack(t);
            case Ease::EaseOutBack:      return EaseOutBack(t);
            case Ease::EaseInOutBack:    return EaseInOutBack(t);
            default:                     return t;
        }
    }
}
