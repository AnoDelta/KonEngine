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
    Sprite2D* target = nullptr; // sprite driven by sheet-frame animations
    Node2D*   node   = nullptr; // node driven by keyframe animations
                                // (can point to the same object if it's a Sprite2D)
    float speed = 1.0f;

    AnimationPlayer(const std::string& name = "AnimationPlayer") : Node(name) {}

    // --- Registration ---

    AnimationPlayer& Add(const Animation& anim) {
        animations[anim.name] = anim;
        return *this;
    }

    // Load compiled .animb file
    bool LoadFromFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[AnimationPlayer] Failed to open: " << path << "\n";
            return false;
        }

        // --- Binary format ---
        // [uint32] animation count
        // Per animation:
        //   [uint32] name length + chars
        //   [uint8]  loop
        //   [uint32] sprite frame count
        //   Per frame: [float x5] srcX srcY srcW srcH duration
        //   [uint32] track count
        //   Per track:
        //     [uint32] name length + chars
        //     [uint32] keyframe count
        //     Per keyframe: [float] time, value, [uint32] curve enum

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

            // Sprite frames
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

            // Keyframe tracks
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
        if (current == animName && playing) return;

        current      = animName;
        currentFrame = 0;
        elapsed      = 0.0f;
        playing      = true;
        finished     = false;

        ApplySpriteFrame();
    }

    void Stop()   { playing = false; finished = true; elapsed = 0.0f; }
    void Pause()  { playing = false; }
    void Resume() { if (!finished) playing = true; }

    void SetLoop(const std::string& animName, bool loop) {
        auto it = animations.find(animName);
        if (it != animations.end()) it->second.loop = loop;
    }

    bool        IsPlaying()       const { return playing; }
    bool        IsFinished()      const { return finished; }
    const std::string& GetCurrent() const { return current; }
    int         GetCurrentFrame() const { return currentFrame; }
    float       GetElapsed()      const { return elapsed; }

    // --- Node update ---

    void Update(float dt) override {
        if (!playing || current.empty()) return;

        auto it = animations.find(current);
        if (it == animations.end()) return;
        Animation& anim = it->second;

        elapsed += dt * speed;

        // --- Sprite sheet ---
        if (!anim.frames.empty())
            TickSpriteFrames(anim);

        // --- Keyframe tracks ---
        if (!anim.tracks.empty() && node)
            ApplyTracks(anim);

        // --- End of animation ---
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

    // Advance sprite sheet frame based on elapsed time
    void TickSpriteFrames(Animation& anim) {
        float acc = 0.0f;
        int newFrame = 0;
        float t = anim.loop
            ? std::fmod(elapsed, anim.duration)
            : std::min(elapsed, anim.duration);

        for (int i = 0; i < static_cast<int>(anim.frames.size()); i++) {
            acc += anim.frames[i].duration;
            if (t < acc) { newFrame = i; break; }
            newFrame = static_cast<int>(anim.frames.size()) - 1;
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

        const AnimationFrame& f = it->second.frames[currentFrame];
        target->srcX         = f.srcX;
        target->srcY         = f.srcY;
        target->srcWidth     = f.srcWidth;
        target->srcHeight    = f.srcHeight;
        target->width        = f.srcWidth;
        target->height       = f.srcHeight;
        target->useSourceRect = true;
    }

    // Sample all keyframe tracks and write to node properties
    void ApplyTracks(Animation& anim) {
        for (auto& track : anim.tracks) {
            float value = track.Sample(elapsed);

            if      (track.name == "x")        node->x        = value;
            else if (track.name == "y")        node->y        = value;
            else if (track.name == "scaleX")   node->scaleX   = value;
            else if (track.name == "scaleY")   node->scaleY   = value;
            else if (track.name == "rotation") node->rotation = value;
            else if (track.name == "alpha") {
                // Works if node is a Sprite2D
                if (auto* s = dynamic_cast<Sprite2D*>(node))
                    s->tint.a = value;
            }
        }
    }
};
