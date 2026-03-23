#include "time.hpp"
#include <thread>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static float deltaTime = 0.0f;
static float targetFrameTime = 0.0f;
static TimePoint lastFrame = Clock::now();

void SetTargetFPS(int fps) {
	targetFrameTime = 1.0f / fps;
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
}

float GetDeltaTime() {
	return deltaTime;
}
