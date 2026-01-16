// Test Input Injection for Afterhours
// This workaround intercepts raylib input functions to enable automated E2E testing.
//
// USAGE:
// 1. Include this header instead of (or after) raylib
// 2. Use macros to redirect raylib input calls through our test layer
// 3. In test mode, push simulated input; otherwise falls through to real raylib
//
// INTEGRATION POINT FOR AFTERHOURS:
// Afterhours should provide hooks in its input system to allow test injection
// without requiring macro interception of raylib calls.

#pragma once

#include <optional>
#include <queue>

// Forward declare raylib types (or include your raylib wrapper)
// For this example, we use simple types
struct Vector2 { float x, y; };

namespace test_input {

// Represents a key press or character input
struct KeyPress {
    int key = 0;
    bool is_char = false;
    char char_value = 0;
};

// Mouse state for simulation
struct MouseState {
    std::optional<Vector2> position;
    bool left_button_held = false;
    bool left_button_pressed_this_frame = false;
    bool left_button_released_this_frame = false;
    bool simulation_active = false;
};

// Global state
extern std::queue<KeyPress> input_queue;
extern bool test_mode;
extern MouseState mouse_state;

// Queue a key press for the next frame
inline void push_key(int key) {
    KeyPress press;
    press.key = key;
    press.is_char = false;
    input_queue.push(press);
}

// Queue a character input
inline void push_char(char c) {
    KeyPress press;
    press.key = 0;
    press.is_char = true;
    press.char_value = c;
    input_queue.push(press);
}

// Clear all queued input
inline void clear_queue() {
    while (!input_queue.empty()) {
        input_queue.pop();
    }
}

// Call at start of each frame to reset per-frame state
inline void reset_frame() {
    mouse_state.left_button_pressed_this_frame = false;
    mouse_state.left_button_released_this_frame = false;
}

// Set simulated mouse position
inline void set_mouse_position(Vector2 pos) {
    mouse_state.position = pos;
    mouse_state.simulation_active = true;
}

// Simulate mouse button press
inline void simulate_mouse_button_press(int button) {
    if (button == 0) {  // Left button
        mouse_state.left_button_held = true;
        mouse_state.left_button_pressed_this_frame = true;
        mouse_state.simulation_active = true;
    }
}

// Simulate mouse button release
inline void simulate_mouse_button_release(int button) {
    if (button == 0) {  // Left button
        mouse_state.left_button_held = false;
        mouse_state.left_button_released_this_frame = true;
        mouse_state.simulation_active = true;
    }
}

// Reset all mouse simulation state
inline void clear_mouse_simulation() {
    mouse_state = MouseState{};
}

//-----------------------------------------------------------------------------
// Input query functions - use these instead of raylib functions
// When test_mode is enabled, these return simulated values
//-----------------------------------------------------------------------------

// Replacement for IsKeyPressed - checks simulated queue first
inline bool is_key_pressed(int key, bool (*real_is_key_pressed)(int)) {
    if (!test_mode || input_queue.empty()) {
        return real_is_key_pressed(key);
    }
    if (!input_queue.front().is_char && input_queue.front().key == key) {
        input_queue.pop();
        return true;
    }
    return real_is_key_pressed(key);
}

// Replacement for GetCharPressed - returns simulated char first
inline int get_char_pressed(int (*real_get_char_pressed)()) {
    if (!test_mode || input_queue.empty()) {
        return real_get_char_pressed();
    }
    if (input_queue.front().is_char) {
        char c = input_queue.front().char_value;
        input_queue.pop();
        return static_cast<int>(c);
    }
    return real_get_char_pressed();
}

// Replacement for GetMousePosition
inline Vector2 get_mouse_position(Vector2 (*real_get_mouse_position)()) {
    if (test_mode && mouse_state.simulation_active && mouse_state.position.has_value()) {
        return mouse_state.position.value();
    }
    return real_get_mouse_position();
}

// Replacement for IsMouseButtonPressed
inline bool is_mouse_button_pressed(int button, bool (*real_fn)(int)) {
    if (test_mode && mouse_state.simulation_active && button == 0) {
        return mouse_state.left_button_pressed_this_frame;
    }
    return real_fn(button);
}

// Replacement for IsMouseButtonDown
inline bool is_mouse_button_down(int button, bool (*real_fn)(int)) {
    if (test_mode && mouse_state.simulation_active && button == 0) {
        return mouse_state.left_button_held;
    }
    return real_fn(button);
}

// Replacement for IsMouseButtonReleased
inline bool is_mouse_button_released(int button, bool (*real_fn)(int)) {
    if (test_mode && mouse_state.simulation_active && button == 0) {
        return mouse_state.left_button_released_this_frame;
    }
    return real_fn(button);
}

//-----------------------------------------------------------------------------
// Convenience functions for common test actions
//-----------------------------------------------------------------------------
inline void simulate_tab() { push_key(258); }  // KEY_TAB
inline void simulate_enter() { push_key(257); }  // KEY_ENTER
inline void simulate_escape() { push_key(256); }  // KEY_ESCAPE

}  // namespace test_input

//-----------------------------------------------------------------------------
// MACRO INTERCEPTION (Optional)
// Define these after including raylib to redirect input calls
//-----------------------------------------------------------------------------
// #define IsKeyPressed(k) test_input::is_key_pressed(k, raylib::IsKeyPressed)
// #define GetCharPressed() test_input::get_char_pressed(raylib::GetCharPressed)
// #define GetMousePosition() test_input::get_mouse_position(raylib::GetMousePosition)
// #define IsMouseButtonPressed(b) test_input::is_mouse_button_pressed(b, raylib::IsMouseButtonPressed)
// #define IsMouseButtonDown(b) test_input::is_mouse_button_down(b, raylib::IsMouseButtonDown)
// #define IsMouseButtonReleased(b) test_input::is_mouse_button_released(b, raylib::IsMouseButtonReleased)

