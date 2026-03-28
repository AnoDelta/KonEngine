#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "../color/color.hpp"
#include "curves.hpp"

// ---------------------------------------------------------------------------
// Sprite sheet frame (snaps — no curves needed)
// ---------------------------------------------------------------------------

struct AnimationFrame {
    float srcX, srcY;
    float srcWidth, srcHeight;
    float duration = 0.1f;

    AnimationFrame(float srcX, float srcY, float srcWidth, float srcHeight, float duration = 0.1f)
        : srcX(srcX), srcY(srcY), srcWidth(srcWidth), srcHeight(srcHeight), duration(duration) {}
};

// ---------------------------------------------------------------------------
// Keyframe — a value at a point in time, with a curve INTO the next keyframe
// ---------------------------------------------------------------------------

struct Keyframe {
    float time  = 0.0f;
    float value = 0.0f;
    Ease  curve = Ease::EaseInOut; // how to travel FROM this key TO the next

    Keyframe(float time, float value, Ease curve = Ease::EaseInOut)
        : time(time), value(value), curve(curve) {}
};

struct KeyframeTrack {
    std::string            name;       // "x", "y", "scaleX", "scaleY", "rotation", "alpha"
    std::vector<Keyframe>  keyframes;

    KeyframeTrack() = default;
    KeyframeTrack(const std::string& name) : name(name) {}

    KeyframeTrack& AddKey(float time, float value, Ease curve = Ease::EaseInOut) {
        keyframes.push_back({ time, value, curve });
        return *this;
    }

    // Sample the interpolated value at a given time
    float Sample(float time) const {
        if (keyframes.empty()) return 0.0f;
        if (keyframes.size() == 1) return keyframes[0].value;

        if (time <= keyframes.front().time) return keyframes.front().value;
        if (time >= keyframes.back().time)  return keyframes.back().value;

        for (size_t i = 0; i + 1 < keyframes.size(); i++) {
            const Keyframe& a = keyframes[i];
            const Keyframe& b = keyframes[i + 1];

            if (time >= a.time && time <= b.time) {
                float span = b.time - a.time;
                if (span == 0.0f) return b.value;

                float t          = (time - a.time) / span; // raw 0..1
                float curved_t   = Curves::Apply(a.curve, t); // reshaped
                return a.value + (b.value - a.value) * curved_t;
            }
        }
        return keyframes.back().value;
    }
};

// ---------------------------------------------------------------------------
// Animation clip
// ---------------------------------------------------------------------------

struct Animation {
    std::string name;
    float       duration     = 0.0f;
    bool        loop         = false;
    float       displayW     = 0.0f;  // from .anim display field (0 = use srcW)
    float       displayH     = 0.0f;  // from .anim display field (0 = use srcH)
    float       displayScale = 1.0f;

    std::vector<AnimationFrame> frames; // sprite sheet frames
    std::vector<KeyframeTrack>  tracks; // property keyframe tracks

    Animation() = default;
    Animation(const std::string& name, bool loop = false)
        : name(name), loop(loop) {}

    // --- Sprite sheet ---
    Animation& AddFrame(float srcX, float srcY, float srcW, float srcH, float dur = 0.1f) {
        frames.push_back({ srcX, srcY, srcW, srcH, dur });
        float total = 0.0f;
        for (auto& f : frames) total += f.duration;
        duration = total;
        return *this;
    }

    // --- Keyframe tracks ---
    KeyframeTrack& Track(const std::string& trackName) {
        for (auto& t : tracks)
            if (t.name == trackName) return t;
        tracks.push_back(KeyframeTrack(trackName));
        return tracks.back();
    }

    // Extend duration to cover the last keyframe across all tracks
    void AutoDuration() {
        for (auto& track : tracks)
            if (!track.keyframes.empty())
                duration = std::max(duration, track.keyframes.back().time);
    }
};
