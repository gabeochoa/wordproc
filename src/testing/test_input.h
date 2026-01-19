// Wrapper that uses afterhours E2E testing implementation
// This file maintains backward compatibility with existing code.
// Note: test_input_fwd.h is included first from external.h for macro wrappers

#pragma once

#include <afterhours/src/plugins/e2e_testing/test_input.h>
#include <afterhours/src/plugins/e2e_testing/input_injector.h>
#include <afterhours/src/plugins/e2e_testing/visible_text.h>
#include "../rl.h"
#include "test_input_fwd.h"  // For test_input_vec2 struct

namespace test_input {

// Re-export types from afterhours
using KeyPress = afterhours::testing::test_input::KeyPress;
using MouseState = afterhours::testing::input_injector::detail::MouseState;

// Test mode functions (access detail::test_mode directly)
inline void set_test_mode(bool enabled) { 
    afterhours::testing::test_input::detail::test_mode = enabled; 
}
inline bool is_test_mode() { 
    return afterhours::testing::test_input::detail::test_mode; 
}

inline void push_key(int key) { 
    afterhours::testing::test_input::push_key(key); 
}
inline void push_char(char c) { 
    afterhours::testing::test_input::push_char(c); 
}
inline void clear_queue() { 
    afterhours::testing::test_input::clear_queue(); 
}
inline void reset_frame() { 
    afterhours::testing::test_input::reset_frame(); 
}

inline void set_mouse_position(vec2 pos) {
    afterhours::testing::test_input::set_mouse_position(pos.x, pos.y);
}
inline void simulate_mouse_button_press(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT) {
        afterhours::testing::test_input::simulate_mouse_press();
    }
}
inline void simulate_mouse_button_release(int button) {
    (void)button;
    afterhours::testing::test_input::simulate_mouse_release();
}
inline void simulate_mouse_press() {
    afterhours::testing::test_input::simulate_mouse_press();
}
inline void simulate_mouse_release() {
    afterhours::testing::test_input::simulate_mouse_release();
}
inline void clear_mouse_simulation() {
    afterhours::testing::test_input::reset_all();
}

inline vec2 get_mouse_position() {
    if (is_test_mode()) {
        float x, y;
        afterhours::testing::input_injector::get_mouse_position(x, y);
        return {x, y};
    }
    return raylib::GetMousePosition_Real();
}

inline test_input_vec2 get_mouse_position_fwd() {
    vec2 pos = get_mouse_position();
    return test_input_vec2{pos.x, pos.y};
}

inline float get_mouse_wheel_move() {
    // Mouse wheel not simulated in tests
    if (is_test_mode()) {
        return 0.0f;
    }
    return raylib::GetMouseWheelMove_Real();
}
inline bool is_mouse_button_pressed(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT && is_test_mode()) {
        return afterhours::testing::input_injector::is_mouse_button_pressed();
    }
    return raylib::IsMouseButtonPressed_Real(button);
}
inline bool is_mouse_button_down(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT && is_test_mode()) {
        return afterhours::testing::input_injector::is_mouse_button_down();
    }
    return raylib::IsMouseButtonDown_Real(button);
}
inline bool is_mouse_button_released(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT && is_test_mode()) {
        return afterhours::testing::input_injector::is_mouse_button_released();
    }
    return raylib::IsMouseButtonReleased_Real(button);
}
inline bool is_mouse_button_up(int button) {
    return !is_mouse_button_down(button);
}

inline bool is_key_pressed(int key) {
    return afterhours::testing::test_input::is_key_pressed(
        key, [](int k) { return raylib::IsKeyPressed_Real(k); });
}
inline int get_char_pressed() {
    return afterhours::testing::test_input::get_char_pressed(
        []() { return raylib::GetCharPressed_Real(); });
}

// Convenience helpers
inline void simulate_tab() { push_key(raylib::KEY_TAB); }
inline void simulate_shift_tab() { 
    afterhours::testing::input_injector::set_key_down(raylib::KEY_LEFT_SHIFT);
    push_key(raylib::KEY_TAB); 
}
inline void simulate_arrow_key(int arrow_key) { push_key(arrow_key); }
inline void simulate_enter() { push_key(raylib::KEY_ENTER); }
inline void simulate_escape() { push_key(raylib::KEY_ESCAPE); }

// UIContext integration (forward to provider if available)
inline void queue_ui_action(int action) { (void)action; /* handled by TestInputProvider */ }
inline void hold_ui_action(int action) { (void)action; }
inline void release_ui_action(int action) { (void)action; }

// Globals for backward compatibility
// Use set_test_mode(bool) and is_test_mode() instead of direct assignment
inline bool& test_mode = afterhours::testing::test_input::detail::test_mode;
inline bool slow_test_mode = false;

inline bool key_consumed_this_frame = false;
inline bool char_consumed_this_frame = false;
inline MouseState mouse_state;

// Visible text registry compatibility
inline void registerVisibleText(const std::string& text) {
    afterhours::testing::VisibleTextRegistry::instance().register_text(text);
}
inline void clearVisibleTextRegistry() {
    afterhours::testing::VisibleTextRegistry::instance().clear();
}

// VisibleTextRegistry class for backward compatibility
class VisibleTextRegistry {
public:
    static VisibleTextRegistry& instance() {
        static VisibleTextRegistry inst;
        return inst;
    }
    void clear() { afterhours::testing::VisibleTextRegistry::instance().clear(); }
    void registerText(const std::string& t) { afterhours::testing::VisibleTextRegistry::instance().register_text(t); }
    bool containsText(const std::string& needle) const { return afterhours::testing::VisibleTextRegistry::instance().contains(needle); }
    std::string getAllText() const { return afterhours::testing::VisibleTextRegistry::instance().get_all(); }
private:
    VisibleTextRegistry() = default;
};

}  // namespace test_input

// input_injector namespace compatibility
namespace input_injector {
    using afterhours::testing::input_injector::set_key_down;
    using afterhours::testing::input_injector::set_key_up;
    using afterhours::testing::input_injector::is_key_down;
    using afterhours::testing::input_injector::consume_press;
    using afterhours::testing::input_injector::hold_key_for_duration;
    using afterhours::testing::input_injector::update_key_hold;
    using afterhours::testing::input_injector::reset_frame;
    
    inline bool is_key_synthetically_down(int key) { 
        return afterhours::testing::input_injector::is_key_down(key); 
    }
    inline bool consume_synthetic_press(int key) {
        return afterhours::testing::input_injector::consume_press(key);
    }
    inline void set_mouse_position(int x, int y) {
        afterhours::testing::input_injector::set_mouse_position(
            static_cast<float>(x), static_cast<float>(y));
    }
    inline void schedule_mouse_click_at(const raylib::Rectangle& rect) {
        afterhours::testing::input_injector::schedule_click_at(
            rect.x, rect.y, rect.width, rect.height);
    }
    inline void inject_scheduled_click() {
        afterhours::testing::input_injector::inject_scheduled_click();
    }
    inline void release_scheduled_click() {
        afterhours::testing::input_injector::release_scheduled_click();
    }
}
