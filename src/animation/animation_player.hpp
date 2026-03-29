#pragma once

#include "animation.hpp"
#include "../node/node.hpp"
#include "../node/node2d.hpp"
#include "../node/sprite2d.hpp"
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "../asset_manager.hpp"

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
		auto data = AssetManager::readFile(path);
		if (data.empty()) {
			std::cerr << "[AnimationPlayer] Failed to open: " << path << "\n";
			return false;
		}

		// Read from memory buffer instead of file stream
		size_t pos = 0;
		auto readU8  = [&]() -> uint8_t  { return data[pos++]; };
		auto readU32 = [&]() -> uint32_t {
			uint32_t v; memcpy(&v, data.data() + pos, 4); pos += 4; return v;
		};
		auto readF32 = [&]() -> float {
			float v; memcpy(&v, data.data() + pos, 4); pos += 4; return v;
		};

		uint32_t animCount = readU32();

		for (uint32_t i = 0; i < animCount; i++) {
			Animation anim;

			uint32_t nameLen = readU32();
			anim.name.resize(nameLen);
			memcpy(anim.name.data(), data.data() + pos, nameLen); pos += nameLen;

			anim.loop = readU8() != 0;

			anim.displayW     = readF32();
			anim.displayH     = readF32();
			anim.displayScale = readF32();

			uint32_t frameCount = readU32();
			for (uint32_t j = 0; j < frameCount; j++) {
				float srcX = readF32(), srcY = readF32();
				float srcW = readF32(), srcH = readF32();
				float dur  = readF32();
				anim.AddFrame(srcX, srcY, srcW, srcH, dur);
			}

			uint32_t trackCount = readU32();
			for (uint32_t j = 0; j < trackCount; j++) {
				uint32_t tNameLen = readU32();
				std::string tName(tNameLen, '\0');
				memcpy(tName.data(), data.data() + pos, tNameLen); pos += tNameLen;

				KeyframeTrack& track = anim.Track(tName);

				uint32_t keyCount = readU32();
				for (uint32_t k = 0; k < keyCount; k++) {
					float time    = readF32();
					float value   = readF32();
					uint32_t curveID = readU32();
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

        // Auto-detect target/node from parent Sprite2D if not explicitly set
        if (!target || !node) {
            if (auto* p = dynamic_cast<Sprite2D*>(parent)) {
                if (!target) target = p;
                if (!node)   node   = p;
            }
        }
        if (!target && !node) {
            std::cerr << "[AnimationPlayer] No target — add as child of a Sprite2D\n";
        }

        if (current == animName && playing) return;

        current      = animName;
        currentFrame = 0;
        elapsed      = 0.0f;
        playing      = true;
        finished     = false;

        // Enable source rect on target automatically
        if (target)
            target->useSourceRect = true;

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

        const Animation&      anim = it->second;
        const AnimationFrame& f    = anim.frames[currentFrame];

        target->srcX          = f.srcX;
        target->srcY          = f.srcY;
        target->srcWidth      = f.srcWidth;
        target->srcHeight     = f.srcHeight;
        target->useSourceRect = true;

        // Use displayW/H from the clip if set, otherwise fall back to frame size.
        // displayScale lets artists author at 1x and scale up in-engine.
        float dW = (anim.displayW > 0.0f ? anim.displayW : f.srcWidth)  * anim.displayScale;
        float dH = (anim.displayH > 0.0f ? anim.displayH : f.srcHeight) * anim.displayScale;
        target->width  = dW;
        target->height = dH;
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
