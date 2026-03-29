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
