#include "input.hpp"
#include <GLFW/glfw3.h>

static GLFWwindow* inputWindow = nullptr;

// Previous frame states
static bool prevKeys[GLFW_KEY_LAST] = {};
static bool prevMouseButtons[GLFW_MOUSE_BUTTON_LAST] = {};
static bool prevGamepadButtons[4][15] = {};
static float mouseScrollY = 0.0f;
static float lastMouseX = 0.0f, lastMouseY = 0.0f;
static float mouseDeltaX = 0.0f, mouseDeltaY = 0.0f;

void InitInput(GLFWwindow* window) {
    inputWindow = window;

    glfwSetScrollCallback(window, [](GLFWwindow*, double, double yOffset) {
        mouseScrollY = (float)yOffset;
    });
}

void UpdateInput() {
    // Update previous key states
    for (int i = 0; i < GLFW_KEY_LAST; i++)
        prevKeys[i] = glfwGetKey(inputWindow, i) == GLFW_PRESS;

    // Update previous mouse button states
    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
        prevMouseButtons[i] = glfwGetMouseButton(inputWindow, i) == GLFW_PRESS;

    // Update previous gamepad states
    for (int p = 0; p < 4; p++) {
        if (!glfwJoystickIsGamepad(p)) continue;
        GLFWgamepadstate state;
        if (glfwGetGamepadState(p, &state))
            for (int b = 0; b < 15; b++)
                prevGamepadButtons[p][b] = state.buttons[b] == GLFW_PRESS;
    }

    // Mouse delta
    double mx, my;
    glfwGetCursorPos(inputWindow, &mx, &my);
    mouseDeltaX = (float)mx - lastMouseX;
    mouseDeltaY = (float)my - lastMouseY;
    lastMouseX = (float)mx;
    lastMouseY = (float)my;

    mouseScrollY = 0.0f; // reset each frame
}

// Keyboard
bool IsKeyDown(Key::Code key) {
    return glfwGetKey(inputWindow, key) == GLFW_PRESS;
}
bool IsKeyPressed(Key::Code key) {
    return glfwGetKey(inputWindow, key) == GLFW_PRESS && !prevKeys[key];
}
bool IsKeyReleased(Key::Code key) {
    return glfwGetKey(inputWindow, key) == GLFW_RELEASE && prevKeys[key];
}

// Mouse
bool IsMouseButtonDown(Mouse::Button button) {
    return glfwGetMouseButton(inputWindow, button) == GLFW_PRESS;
}
bool IsMouseButtonPressed(Mouse::Button button) {
    return glfwGetMouseButton(inputWindow, button) == GLFW_PRESS && !prevMouseButtons[button];
}
bool IsMouseButtonReleased(Mouse::Button button) {
    return glfwGetMouseButton(inputWindow, button) == GLFW_RELEASE && prevMouseButtons[button];
}
float GetMouseX() { return lastMouseX; }
float GetMouseY() { return lastMouseY; }
float GetMouseDeltaX() { return mouseDeltaX; }
float GetMouseDeltaY() { return mouseDeltaY; }
float GetMouseScroll() { return mouseScrollY; }

// Gamepad
bool IsGamepadConnected(int player) {
    return glfwJoystickIsGamepad(player);
}
bool IsGamepadButtonDown(int player, Gamepad::Button button) {
    GLFWgamepadstate state;
    return glfwGetGamepadState(player, &state) && state.buttons[button] == GLFW_PRESS;
}
bool IsGamepadButtonPressed(int player, Gamepad::Button button) {
    GLFWgamepadstate state;
    return glfwGetGamepadState(player, &state) &&
    state.buttons[button] == GLFW_PRESS && !prevGamepadButtons[player][button];
}
bool IsGamepadButtonReleased(int player, Gamepad::Button button) {
    GLFWgamepadstate state;
    return glfwGetGamepadState(player, &state) &&
    state.buttons[button] == GLFW_RELEASE && prevGamepadButtons[player][button];
}
float GetGamepadAxis(int player, Gamepad::Axis axis) {
    GLFWgamepadstate state;
    if (glfwGetGamepadState(player, &state))
        return state.axes[axis];
    return 0.0f;
}
