#pragma once

// Centralized input functions that route through afterhours/test_input
// This ensures all input goes through test-aware wrappers for E2E testing
//
// Usage: Replace raylib::IsKeyPressed(key) with input::isKeyPressed(key)
//        Replace raylib::GetMousePosition() with input::getMousePosition()

#include "../external.h"  // For test_input:: namespace and afterhours types
#include <afterhours/src/plugins/input_system.h>

namespace input {

// ============================================================
// Keyboard Input (test-aware via test_input)
// ============================================================

inline bool isKeyPressed(int key) {
    return test_input::is_key_pressed(key);
}

inline bool isKeyDown(int key) {
    return test_input::is_key_down(key);
}

inline int getCharPressed() {
    return test_input::get_char_pressed();
}

// ============================================================
// Mouse Input (test-aware via test_input)
// ============================================================

inline raylib::Vector2 getMousePosition() {
    return test_input::get_mouse_position();
}

inline bool isMouseButtonPressed(int button) {
    return test_input::is_mouse_button_pressed(button);
}

inline bool isMouseButtonDown(int button) {
    return test_input::is_mouse_button_down(button);
}

inline bool isMouseButtonReleased(int button) {
    return test_input::is_mouse_button_released(button);
}

inline bool isMouseButtonUp(int button) {
    return test_input::is_mouse_button_up(button);
}

inline float getMouseWheelMove() {
    return test_input::get_mouse_wheel_move();
}

// ============================================================
// Afterhours Input Integration
// ============================================================

// Type aliases for afterhours input types
using ValidInputs = afterhours::input::ValidInputs;
using AnyInput = afterhours::input::AnyInput;

}  // namespace input

