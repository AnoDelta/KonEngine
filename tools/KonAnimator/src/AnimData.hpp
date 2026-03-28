#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

// -----------------------------------------------------------------------
// Ease enum — must match KonEngine curves.hpp order exactly
// -----------------------------------------------------------------------
enum class Ease : uint32_t {
    Linear,
    EaseIn, EaseOut, EaseInOut,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    EaseInBack, EaseOutBack, EaseInOutBack,
    COUNT
};

static const char* kEaseNames[] = {
    "Linear",
    "EaseIn","EaseOut","EaseInOut",
    "EaseInCubic","EaseOutCubic","EaseInOutCubic",
    "EaseInElastic","EaseOutElastic","EaseInOutElastic",
    "EaseInBounce","EaseOutBounce","EaseInOutBounce",
    "EaseInBack","EaseOutBack","EaseInOutBack",
};
static const char* kEaseLower[] = {
    "linear",
    "easein","easeout","easeinout",
    "easeincubic","easeoutcubic","easeinoutcubic",
    "easeinelastic","easeoutelastic","easeinoutelastic",
    "easeinbounce","easeoutbounce","easeinoutbounce",
    "easeinback","easeoutback","easeinoutback",
};
static constexpr int kEaseCount = static_cast<int>(Ease::COUNT);

struct AnimFrame {
    float srcX=0, srcY=0, srcW=32, srcH=32, duration=0.1f;
};

struct Keyframe {
    float time=0, value=0;
    Ease  curve = Ease::EaseInOut;
};

struct KFTrack {
    std::string           name;
    std::vector<Keyframe> keys;
    void sortKeys() {
        std::sort(keys.begin(), keys.end(),
            [](const Keyframe& a, const Keyframe& b){ return a.time < b.time; });
    }
};

struct AnimClip {
    std::string name         = "anim";
    bool        loop         = false;
    float       displayW     = 32.0f;
    float       displayH     = 32.0f;
    float       displayScale = 1.0f;

    std::vector<AnimFrame> frames;
    std::vector<KFTrack>   tracks;

    float totalDuration() const {
        float t = 0.0f;
        for (auto& f : frames) t += f.duration;
        for (auto& tr : tracks)
            if (!tr.keys.empty())
                t = std::max(t, tr.keys.back().time);
        return t;
    }

    KFTrack* findTrack(const std::string& n) {
        for (auto& tr : tracks) if (tr.name == n) return &tr;
        return nullptr;
    }
    KFTrack& getOrAddTrack(const std::string& n) {
        if (auto* t = findTrack(n)) return *t;
        tracks.push_back({n, {}});
        return tracks.back();
    }
};

struct AnimProject {
    std::string           filePath;
    std::string           spritesheetPath;
    std::vector<AnimClip> clips;
    bool                  dirty = false;
};
