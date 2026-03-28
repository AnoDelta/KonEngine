#include "time.hpp"
#include <thread>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static float deltaTime = 0.0f;
static float targetFrameTime = 0.0f;
static TimePoint lastFrame = Clock::now();
static float totalTime   = 0.0f;
static int   currentFPS  = 0;
static int   fpsCounter  = 0;
static float fpsTimer    = 0.0f;

void SetTargetFPS(int fps) {
	targetFrameTime = 1.0f / fps;
}

void TickTime() {
	TimePoint currentFrame = Clock::now();
	deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
    totalTime += deltaTime;
    fpsCounter++;
    fpsTimer   += deltaTime;
    if (fpsTimer >= 1.0f) {
        currentFPS = fpsCounter;
        fpsCounter = 0;
        fpsTimer  -= 1.0f;
    }

	if (targetFrameTime > 0.0f && deltaTime < targetFrameTime) {
		float sleepTime = targetFrameTime - deltaTime;
		std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
		deltaTime = targetFrameTime; 
	}

	lastFrame = Clock::now();
}

float GetDeltaTime() {
	return deltaTime;
}

float GetTime() { return totalTime; }

int   GetFPS()  { return currentFPS; }
