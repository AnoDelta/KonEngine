#pragma once

#include <functional>
#include <string>

void SetTargetFPS(int fps);
float GetDeltaTime();
int   GetFPS();
float GetTime();
void  TickTime();

// ---------------------------------------------------------------------------
// Timer -- frame-rate independent timing for gameplay events.
// Uses delta time so behavior is consistent regardless of FPS.
//
// Usage:
//   TimerCreate("spawn", 2.0f, true, []{ SpawnEnemy(); });  // repeating
//   TimerCreate("dash", 0.3f, false, []{ EndDash(); });     // one-shot
//
//   // In game loop:
//   TimerUpdateAll(dt);
//
//   TimerPause("spawn");
//   TimerResume("spawn");
//   TimerRemove("spawn");
//   if (TimerExists("dash")) { ... }
//   float remaining = TimerRemaining("dash");
// ---------------------------------------------------------------------------
void  TimerCreate(const std::string& id, float duration, bool repeating,
                  std::function<void()> callback);
void  TimerRemove(const std::string& id);
void  TimerRemoveAll();
void  TimerPause(const std::string& id);
void  TimerResume(const std::string& id);
void  TimerReset(const std::string& id);
bool  TimerExists(const std::string& id);
bool  TimerFinished(const std::string& id);
float TimerRemaining(const std::string& id);
void  TimerUpdateAll(float dt);
