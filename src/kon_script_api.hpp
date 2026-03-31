#pragma once
// KonScript API bridge — free functions callable from KonScript.
// Include this in your engine's main compiled file (e.g. engine.cpp or main.cpp)
// AFTER the Window instance is created.

#include "window/window.hpp"

// Global window pointer — set this after constructing Window:
//   g_KonWindow = &window;
inline Window* g_KonWindow = nullptr;

// Debug mode flag
inline bool g_KonDebugMode = false;

// SetVSync(true/false) — wraps window.setVsync()
inline void SetVSync(bool enabled) {
    if (g_KonWindow) g_KonWindow->setVsync(enabled);
}

// DebugMode(true/false) — enables engine debug overlay
inline void DebugMode(bool enabled) {
    g_KonDebugMode = enabled;
}

// IsDebugMode() — returns current debug state
inline bool IsDebugMode() {
    return g_KonDebugMode;
}

// GetWindowWidth / GetWindowHeight
inline int GetWindowWidth() {
    return g_KonWindow ? g_KonWindow->getWidth() : 0;
}
inline int GetWindowHeight() {
    return g_KonWindow ? g_KonWindow->getHeight() : 0;
}
