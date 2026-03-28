#pragma once

#include <GLFW/glfw3.h>

namespace Key {
    enum Code {
        // Letters
        A=65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers
        Num0=48, Num1, Num2, Num3, Num4,
        Num5, Num6, Num7, Num8, Num9,

        // Arrow keys
        Right=262, Left, Down, Up,

        // Special
        Space=32, Enter=257, Escape=256, Esc=256, Tab=258,
        Backspace=259, Shift=340, Ctrl=341, Alt=342,

        // Function keys
        F1=290, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12
    };
}

bool IsKeyDown(Key::Code key);
bool IsKeyPressed(Key::Code key);
bool IsKeyReleased(Key::Code key);

namespace Mouse {
    enum Button {
        Left = 0,
        Right = 1,
        Middle = 2
    };
}

bool IsMouseButtonDown(Mouse::Button button);
bool IsMouseButtonPressed(Mouse::Button button);
bool IsMouseButtonReleased(Mouse::Button button);
float GetMouseX();
float GetMouseY();
float GetMouseDeltaX();
float GetMouseDeltaY();
float GetMouseScroll();

namespace Gamepad {
    enum Button {
        A = 0, B = 1, X = 2, Y = 3,
        LeftBumper = 4, RightBumper = 5,
        Back = 6, Start = 7,
        LeftThumb = 8, RightThumb = 9,
        DPadUp = 10, DPadRight = 11,
        DPadDown = 12, DPadLeft = 13
    };

    enum Axis {
        LeftX = 0, LeftY = 1,
        RightX = 2, RightY = 3,
        LeftTrigger = 4, RightTrigger = 5
    };
}

bool IsGamepadConnected(int player);
bool IsGamepadButtonDown(int player, Gamepad::Button button);
bool IsGamepadButtonPressed(int player, Gamepad::Button button);
bool IsGamepadButtonReleased(int player, Gamepad::Button button);
float GetGamepadAxis(int player, Gamepad::Axis axis);

void InitInput(GLFWwindow* window);
void UpdateInput();
