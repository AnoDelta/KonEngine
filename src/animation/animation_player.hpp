#pragma once

#include "animation.hpp"
#include "../node/node.hpp"
#include "../node/node2d.hpp"
#include "../node/sprite2d.hpp"
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>

class AnimationPlayer : public Node {
public:
    Sprite2D* target = nullptr;
    Node2D*   node   = nullptr;
    float speed = 1.0f;

    AnimationPlayer(const std::string& name = "AnimationPlayer") : Node(name) {}

    // --- Registration ---

    AnimationPlayer& Add(const Animation& anim) {
        animations[anim.name] = anim;
        return *this;
    }

    bool LoadFromFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[AnimationPlayer] Failed to open: " << path << "\n";
            return false;
        }

        uint32_t animCount = 0;
        file.read(reinterpret_cast<char*>(&animCount), sizeof(animCount));

        for (uint32_t i = 0; i < animCount; i++) {
            Animation anim;

            uint32_t nameLen = 0;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            anim.name.resize(nameLen);
            file.read(anim.name.data(), nameLen);

            uint8_t loop = 0;
            file.read(reinterpret_cast<char*>(&loop), sizeof(loop));
            anim.loop = loop != 0;

            file.read(reinterpret_cast<char*>(&anim.displayW),     sizeof(float));
            file.read(reinterpret_cast<char*>(&anim.displayH),     sizeof(float));
            file.read(reinterpret_cast<char*>(&anim.displayScale), sizeof(float));

            uint32_t frameCount = 0;
            file.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
            for (uint32_t j = 0; j < frameCount; j++) {
                float srcX, srcY, srcW, srcH, dur;
                file.read(reinterpret_cast<char*>(&srcX), sizeof(float));
                file.read(reinterpret_cast<char*>(&srcY), sizeof(float));
                file.read(reinterpret_cast<char*>(&srcW), sizeof(float));
                file.read(reinterpret_cast<char*>(&srcH), sizeof(float));
                file.read(reinterpret_cast<char*>(&dur),  sizeof(float));
                anim.AddFrame(srcX, srcY, srcW, srcH, dur);
            }

            uint32_t trackCount = 0;
            file.read(reinterpret_cast<char*>(&trackCount), sizeof(trackCount));
            for (uint32_t j = 0; j < trackCount; j++) {
                uint32_t tNameLen = 0;
                file.read(reinterpret_cast<char*>(&tNameLen), sizeof(tNameLen));
                std::string tName(tNameLen, '\0');
                file.read(tName.data(), tNameLen);

                KeyframeTrack& track = anim.Track(tName);

                uint32_t keyCount = 0;
                file.read(reinterpret_cast<char*>(&keyCount), sizeof(keyCount));
                for (uint32_t k = 0; k < keyCount; k++) {
                    float time, value;
                    uint32_t curveID;
                    file.read(reinterpret_cast<char*>(&time),    sizeof(float));
                    file.read(reinterpret_cast<char*>(&value),   sizeof(float));
                    file.read(reinterpret_cast<char*>(&curveID), sizeof(uint32_t));
                    track.AddKey(time, value, static_cast<Ease>(curveID));
                }
            }

            anim.AutoDuration();
            animations[anim.name] = anim;
        }

        return true;
    }

    // --- Playback ---

    void Play(const std::string& animName) {
        auto it = animations.find(animName);
        if (it == animations.end()) {
            std::cerr << "[AnimationPlayer] Not found: " << animName << "\n";
            return;
        }

        if (!target || !node) {
            if (auto* p = dynamic_cast<Sprite2D*>(parent)) {
                if (!target) target = p;
                if (!node)   node   = p;
            }
        }
        if (!target && !node)
            std::cerr << "[AnimationPlayer] No target — add as child of a Sprite2D\n";

        if (current == animName && playing) return;

        // Clean up any visual state left by the previous clip
        UndoPositionDelta();
        RestoreVisualBase();

        current      = animName;
        currentFrame = 0;
        elapsed      = 0.0f;
        playing      = true;
        finished     = false;

        // Snapshot purely-visual properties (scale, rotation, alpha).
        // x/y are NOT snapshotted — gameplay owns those.
        SnapshotVisualBase();

        if (target)
            target->useSourceRect = true;

        ApplySpriteFrame();
    }

    void Stop() {
        UndoPositionDelta();
        RestoreVisualBase();
        playing  = false;
        finished = true;
        elapsed  = 0.0f;
    }

    void Pause()  { playing = false; }
    void Resume() { if (!finished) playing = true; }

    void SetLoop(const std::string& animName, bool loop) {
        auto it = animations.find(animName);
        if (it != animations.end()) it->second.loop = loop;
    }

    bool               IsPlaying()    const { return playing; }
    bool               IsFinished()   const { return finished; }
    const std::string& GetCurrent()   const { return current; }
    int                GetCurrentFrame() const { return currentFrame; }
    float              GetElapsed()   const { return elapsed; }

    // --- Node update ---

    void Update(float dt) override {
        if (!playing || current.empty()) return;

        auto it = animations.find(current);
        if (it == animations.end()) return;
        Animation& anim = it->second;

        elapsed += dt * speed;

        if (!anim.frames.empty())
            TickSpriteFrames(anim);

        if (!anim.tracks.empty() && node)
            ApplyTracks(anim);

        if (elapsed >= anim.duration && anim.duration > 0.0f) {
            if (anim.loop) {
                elapsed = std::fmod(elapsed, anim.duration);
            } else {
                elapsed  = anim.duration;
                playing  = false;
                finished = true;
                Emit("animation_finished");
            }
        }
    }

private:
    std::unordered_map<std::string, Animation> animations;

    std::string current;
    int   currentFrame = 0;
    float elapsed      = 0.0f;
    bool  playing      = false;
    bool  finished     = false;

    // ── Visual-only base (scale, rotation, alpha) ────────────────────────
    // Snapshotted at Play(), restored at Stop()/clip-switch.
    // x/y deliberately excluded — gameplay code owns position.
    bool  hasVisualBase = false;
    float baseScaleX    = 1.0f;
    float baseScaleY    = 1.0f;
    float baseRot       = 0.0f;
    float baseAlpha     = 1.0f;

    void SnapshotVisualBase() {
        if (!node) { hasVisualBase = false; return; }
        hasVisualBase = true;
        baseScaleX    = node->scaleX;
        baseScaleY    = node->scaleY;
        baseRot       = node->rotation;
        if (auto* s = dynamic_cast<Sprite2D*>(node)) baseAlpha = s->tint.a;
        else                                               baseAlpha = 1.0f;
    }

    void RestoreVisualBase() {
        if (!hasVisualBase || !node) return;
        node->scaleX   = baseScaleX;
        node->scaleY   = baseScaleY;
        node->rotation = baseRot;
        if (auto* s = dynamic_cast<Sprite2D*>(node))
            s->tint.a = baseAlpha;
        hasVisualBase = false;
    }

    // ── Position delta tracking ──────────────────────────────────────────
    // x/y tracks add an offset each frame. We track what we applied last
    // frame so we can subtract it before applying the new value.
    // This way the animation never permanently moves the node — it just
    // rides on top of wherever gameplay put it.
    float lastDeltaX = 0.0f;
    float lastDeltaY = 0.0f;

    void UndoPositionDelta() {
        if (!node) return;
        node->x     -= lastDeltaX;
        node->y     -= lastDeltaY;
        lastDeltaX   = 0.0f;
        lastDeltaY   = 0.0f;
    }

    // ── Sprite frame ─────────────────────────────────────────────────────

    void TickSpriteFrames(Animation& anim) {
        float acc = 0.0f;
        int newFrame = 0;
        float t = anim.loop
            ? std::fmod(elapsed, anim.duration)
            : std::min(elapsed, anim.duration);

        for (int i = 0; i < (int)anim.frames.size(); i++) {
            acc += anim.frames[i].duration;
            if (t < acc) { newFrame = i; break; }
            newFrame = (int)anim.frames.size() - 1;
        }

        if (newFrame != currentFrame) {
            currentFrame = newFrame;
            ApplySpriteFrame();
        }
    }

    void ApplySpriteFrame() {
        if (!target || current.empty()) return;
        auto it = animations.find(current);
        if (it == animations.end() || it->second.frames.empty()) return;

        const Animation&      anim = it->second;
        const AnimationFrame& f    = anim.frames[currentFrame];

        target->srcX          = f.srcX;
        target->srcY          = f.srcY;
        target->srcWidth      = f.srcWidth;
        target->srcHeight     = f.srcHeight;
        target->useSourceRect = true;

        float dW = (anim.displayW > 0.0f ? anim.displayW : f.srcWidth)  * anim.displayScale;
        float dH = (anim.displayH > 0.0f ? anim.displayH : f.srcHeight) * anim.displayScale;
        target->width  = dW;
        target->height = dH;
    }

    // ── Track application ────────────────────────────────────────────────
    //
    //  x / y      → delta on top of current gameplay position.
    //               Undo last frame's delta first so it doesn't accumulate.
    //  scaleX/Y   → multiply base scale  (1.0 = no change)
    //  rotation   → add to base rotation (0   = no change)
    //  alpha      → multiply base alpha   (1.0 = no change)

    void ApplyTracks(Animation& anim) {
        // Undo last frame's position delta before re-applying
        if (node) {
            node->x -= lastDeltaX;
            node->y -= lastDeltaY;
        }
        lastDeltaX = 0.0f;
        lastDeltaY = 0.0f;

        for (auto& track : anim.tracks) {
            float value = track.Sample(elapsed);

            if (track.name == "x") {
                if (node) { node->x += value; lastDeltaX = value; }
            }
            else if (track.name == "y") {
                if (node) { node->y += value; lastDeltaY = value; }
            }
            else if (track.name == "scaleX") {
                if (node && hasVisualBase) node->scaleX = baseScaleX * value;
            }
            else if (track.name == "scaleY") {
                if (node && hasVisualBase) node->scaleY = baseScaleY * value;
            }
            else if (track.name == "rotation") {
                if (node && hasVisualBase) node->rotation = baseRot + value;
            }
            else if (track.name == "alpha") {
                if (auto* s = dynamic_cast<Sprite2D*>(node))
                    s->tint.a = hasVisualBase ? baseAlpha * value : value;
            }
        }
    }
};
