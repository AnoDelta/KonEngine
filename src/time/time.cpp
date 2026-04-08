#include "time.hpp"
#include <thread>
#include <chrono>

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static float     deltaTime       = 0.0f;
static float     targetFrameTime = 0.0f;
static int       currentFPS      = 0;
static float     fpsTimer        = 0.0f;
static int       fpsFrameCount   = 0;
static TimePoint lastFrame       = Clock::now();
static TimePoint startTime       = Clock::now();

void SetTargetFPS(int fps) {
    targetFrameTime = (fps > 0) ? 1.0f / fps : 0.0f;
}

void TickTime() {
    TimePoint currentFrame = Clock::now();
    deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();

    if (targetFrameTime > 0.0f && deltaTime < targetFrameTime) {
        float sleepTime = targetFrameTime - deltaTime;
        std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        deltaTime = targetFrameTime;
    }

    lastFrame = Clock::now();

    // FPS counter -- updated once per second
    fpsFrameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0f) {
        currentFPS    = fpsFrameCount;
        fpsFrameCount = 0;
        fpsTimer     -= 1.0f;
    }
}

float GetDeltaTime() { return deltaTime; }
int   GetFPS()       { return currentFPS; }
float GetTime()      {
    return std::chrono::duration<float>(Clock::now() - startTime).count();
}

// ---------------------------------------------------------------------------
// Timer system
// ---------------------------------------------------------------------------
#include <vector>
#include <algorithm>

struct TimerData {
    std::string id;
    float duration;
    float elapsed = 0.0f;
    bool repeating;
    bool paused = false;
    bool finished = false;
    std::function<void()> callback;
};

static std::vector<TimerData> g_timers;

static TimerData* FindTimer(const std::string& id) {
    for (auto& t : g_timers)
        if (t.id == id) return &t;
    return nullptr;
}

void TimerCreate(const std::string& id, float duration, bool repeating,
                 std::function<void()> callback) {
    // Replace if exists
    TimerRemove(id);
    g_timers.push_back({id, duration, 0.0f, repeating, false, false, callback});
}

void TimerRemove(const std::string& id) {
    g_timers.erase(
        std::remove_if(g_timers.begin(), g_timers.end(),
            [&](const TimerData& t) { return t.id == id; }),
        g_timers.end());
}

void TimerRemoveAll() { g_timers.clear(); }

void TimerPause(const std::string& id) {
    auto* t = FindTimer(id);
    if (t) t->paused = true;
}

void TimerResume(const std::string& id) {
    auto* t = FindTimer(id);
    if (t) t->paused = false;
}

void TimerReset(const std::string& id) {
    auto* t = FindTimer(id);
    if (t) { t->elapsed = 0.0f; t->finished = false; }
}

bool TimerExists(const std::string& id) { return FindTimer(id) != nullptr; }

bool TimerFinished(const std::string& id) {
    auto* t = FindTimer(id);
    return t ? t->finished : true;
}

float TimerRemaining(const std::string& id) {
    auto* t = FindTimer(id);
    if (!t) return 0.0f;
    float rem = t->duration - t->elapsed;
    return rem > 0.0f ? rem : 0.0f;
}

void TimerUpdateAll(float dt) {
    for (auto& t : g_timers) {
        if (t.paused || t.finished) continue;
        t.elapsed += dt;
        if (t.elapsed >= t.duration) {
            if (t.callback) t.callback();
            if (t.repeating) {
                t.elapsed -= t.duration; // preserve overshoot for consistency
            } else {
                t.finished = true;
            }
        }
    }
}
