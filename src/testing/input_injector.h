// Wrapper that uses the extracted e2e_testing.h implementation
// This file maintains backward compatibility with existing code.

#pragma once

#include "../extracted/e2e_testing.h"
#include "../rl.h"

namespace input_injector {

// Forward to extracted implementation
inline void set_key_down(int keycode) {
    afterhours::testing::input_injector::set_key_down(keycode);
}
inline void set_key_up(int keycode) {
    afterhours::testing::input_injector::set_key_up(keycode);
}
inline void inject_key_press(int keycode) {
    set_key_down(keycode);
}
inline bool consume_synthetic_press(int keycode) {
    return afterhours::testing::input_injector::consume_press(keycode);
}
inline bool is_key_synthetically_down(int keycode) {
    return afterhours::testing::input_injector::is_key_down(keycode);
}
inline void hold_key_for_duration(int keycode, float duration) {
    afterhours::testing::input_injector::hold_key_for_duration(keycode, duration);
}
inline void update_key_hold(float dt) {
    afterhours::testing::input_injector::update_key_hold(dt);
}

inline void set_mouse_position(int x, int y) {
    afterhours::testing::input_injector::set_mouse_position(
        static_cast<float>(x), static_cast<float>(y));
}
inline vec2 get_mouse_position() {
    float x, y;
    afterhours::testing::input_injector::get_mouse_position(x, y);
    return {x, y};
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

inline bool is_mouse_button_down(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT) {
        return afterhours::testing::input_injector::is_mouse_button_down();
    }
    return raylib::IsMouseButtonDown_Real(button);
}
inline bool is_mouse_button_pressed(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT) {
        return afterhours::testing::input_injector::is_mouse_button_pressed();
    }
    return raylib::IsMouseButtonPressed_Real(button);
}
inline bool is_mouse_button_released(int button) {
    if (button == raylib::MOUSE_BUTTON_LEFT) {
        return afterhours::testing::input_injector::is_mouse_button_released();
    }
    return raylib::IsMouseButtonReleased_Real(button);
}

inline void reset_frame() {
    afterhours::testing::input_injector::reset_frame();
}

}  // namespace input_injector
