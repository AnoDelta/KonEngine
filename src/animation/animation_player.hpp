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

    // ── Overlay reset ─────────────────────────────────────────────────────
    // Animation transforms are now stored as overlays on the node/sprite:
    //   Node2D::animOffsetX/Y   — added to position during rendering
    //   Node2D::animScaleX/Y    — multiplied to scale during rendering
    //   Node2D::animRotation    — added to rotation during rendering
    //   Sprite2D::animAlpha     — multiplied to tint alpha during rendering
    // This never touches the actual x/y/scaleX/scaleY/rotation/alpha.

    void ResetOverlays() {
        if (node) {
            node->animOffsetX  = 0.0f;
            node->animOffsetY  = 0.0f;
            node->animScaleX   = 1.0f;
            node->animScaleY   = 1.0f;
            node->animRotation = 0.0f;
        }
        if (auto* s = dynamic_cast<Sprite2D*>(node))
            s->animAlpha = 1.0f;
    }

    // Legacy stubs for backward compatibility
    void SnapshotVisualBase() {}
    void RestoreVisualBase()  { ResetOverlays(); }
    void UndoPositionDelta()  { ResetOverlays(); }

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

        // Display size is NOT set here — it comes from the sprite's own
        // width/height combined with the animation overlay's animScaleX/Y.
    }

    // ── Track application ────────────────────────────────────────────────
    //
    // All animation values are stored as rendering overlays — the node's
    // actual x/y/scaleX/scaleY/rotation/alpha are NEVER modified.
    //
    //  x / y      → added to position during rendering  (0 = no offset)
    //  scaleX/Y   → multiplied to scale during rendering (1.0 = no change)
    //  rotation   → added to rotation during rendering  (0 = no change)
    //  alpha      → multiplied to alpha during rendering (1.0 = no change)

    void ApplyTracks(Animation& anim) {
        if (!node) return;

        // Reset overlays before applying current frame's values
        node->animOffsetX  = 0.0f;
        node->animOffsetY  = 0.0f;
        node->animScaleX   = 1.0f;
        node->animScaleY   = 1.0f;
        node->animRotation = 0.0f;
        Sprite2D* spr = dynamic_cast<Sprite2D*>(node);
        if (spr) spr->animAlpha = 1.0f;

        for (auto& track : anim.tracks) {
            float value = track.Sample(elapsed);

            if (track.name == "x") {
                node->animOffsetX = value;
            }
            else if (track.name == "y") {
                node->animOffsetY = value;
            }
            else if (track.name == "scaleX") {
                node->animScaleX = value;
            }
            else if (track.name == "scaleY") {
                node->animScaleY = value;
            }
            else if (track.name == "rotation") {
                node->animRotation = value;
            }
            else if (track.name == "alpha") {
                if (spr) spr->animAlpha = value;
            }
        }
    }
};
