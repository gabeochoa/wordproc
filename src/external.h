#pragma once

#include "std_include.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

namespace raylib {
#if defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#elif __has_include("raylib/raylib.h")
#include "raylib/raylib.h"
#include "raylib/raymath.h"
#include "raylib/rlgl.h"
#else
#error "raylib headers not found"
#endif
#else
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#endif

inline bool IsMouseButtonPressed_Real(int button) {
    return IsMouseButtonPressed(button);
}
inline bool IsMouseButtonDown_Real(int button) {
    return IsMouseButtonDown(button);
}
inline bool IsMouseButtonReleased_Real(int button) {
    return IsMouseButtonReleased(button);
}
inline bool IsMouseButtonUp_Real(int button) { return IsMouseButtonUp(button); }
inline int GetCharPressed_Real() { return GetCharPressed(); }
inline bool IsKeyPressed_Real(int key) { return IsKeyPressed(key); }
inline bool IsKeyDown_Real(int key) { return IsKeyDown(key); }
inline Vector2 GetMousePosition_Real() { return GetMousePosition(); }
inline float GetMouseWheelMove_Real() { return GetMouseWheelMove(); }

}  // namespace raylib

// Afterhours E2E testing - direct usage
#include <afterhours/src/plugins/e2e_testing/test_input.h>
#include <afterhours/src/plugins/e2e_testing/input_injector.h>
#include <afterhours/src/plugins/e2e_testing/visible_text.h>

// Namespace aliases for convenience
namespace test_input {
    using namespace afterhours::testing::test_input;
    
    // Convenience accessors
    inline void set_test_mode(bool enabled) { detail::test_mode = enabled; }
    inline bool is_test_mode() { return detail::test_mode; }
    
    // Reference for direct assignment (e.g., test_input::test_mode = true)
    inline bool& test_mode = detail::test_mode;
    
    // Visible text registration
    inline void registerVisibleText(const std::string& text) {
        afterhours::testing::VisibleTextRegistry::instance().register_text(text);
    }
    inline void clearVisibleTextRegistry() {
        afterhours::testing::VisibleTextRegistry::instance().clear();
    }
    
    // Mouse helpers using raylib backend
    inline bool is_mouse_button_pressed(int button) {
        if (button == raylib::MOUSE_BUTTON_LEFT && detail::test_mode) {
            return afterhours::testing::input_injector::is_mouse_button_pressed();
        }
        return raylib::IsMouseButtonPressed_Real(button);
    }
    inline bool is_mouse_button_down(int button) {
        if (button == raylib::MOUSE_BUTTON_LEFT && detail::test_mode) {
            return afterhours::testing::input_injector::is_mouse_button_down();
        }
        return raylib::IsMouseButtonDown_Real(button);
    }
    inline bool is_mouse_button_released(int button) {
        if (button == raylib::MOUSE_BUTTON_LEFT && detail::test_mode) {
            return afterhours::testing::input_injector::is_mouse_button_released();
        }
        return raylib::IsMouseButtonReleased_Real(button);
    }
    inline bool is_mouse_button_up(int button) {
        return !is_mouse_button_down(button);
    }
    
    // Key helpers using raylib backend
    inline bool is_key_pressed(int key) {
        return afterhours::testing::test_input::is_key_pressed(
            key, [](int k) { return raylib::IsKeyPressed_Real(k); });
    }
    inline bool is_key_down(int key) {
        return afterhours::testing::test_input::is_key_down(
            key, [](int k) { return raylib::IsKeyDown_Real(k); });
    }
    inline int get_char_pressed() {
        return afterhours::testing::test_input::get_char_pressed(
            []() { return raylib::GetCharPressed_Real(); });
    }
    
    // Mouse position
    inline raylib::Vector2 get_mouse_position() {
        return afterhours::testing::test_input::get_mouse_position<raylib::Vector2>(
            []() { return raylib::GetMousePosition_Real(); });
    }
    
    inline float get_mouse_wheel_move() {
        if (detail::test_mode) return 0.0f;
        return raylib::GetMouseWheelMove_Real();
    }
}

namespace input_injector {
    using afterhours::testing::input_injector::set_key_down;
    using afterhours::testing::input_injector::set_key_up;
    using afterhours::testing::input_injector::is_key_down;
    using afterhours::testing::input_injector::consume_press;
    using afterhours::testing::input_injector::reset_frame;
    using afterhours::testing::input_injector::reset_all;
    using afterhours::testing::input_injector::set_mouse_position;
    
    // Alias for backward compatibility
    inline bool is_key_synthetically_down(int key) {
        return afterhours::testing::input_injector::is_key_down(key);
    }
}

// Test wrapper functions (global scope for macro compatibility)
inline bool IsMouseButtonPressed_Test(int button) {
    return test_input::is_mouse_button_pressed(button);
}
inline bool IsMouseButtonDown_Test(int button) {
    return test_input::is_mouse_button_down(button);
}
inline bool IsMouseButtonReleased_Test(int button) {
    return test_input::is_mouse_button_released(button);
}
inline bool IsMouseButtonUp_Test(int button) {
    return test_input::is_mouse_button_up(button);
}
inline int GetCharPressed_Test() { return test_input::get_char_pressed(); }
inline bool IsKeyPressed_Test(int key) {
    return test_input::is_key_pressed(key);
}
inline bool IsKeyDown_Test(int key) {
    return test_input::is_key_down(key);
}
inline raylib::Vector2 GetMousePosition_Test() {
    return test_input::get_mouse_position();
}
inline float GetMouseWheelMove_Test() {
    return test_input::get_mouse_wheel_move();
}

// Also add to raylib namespace for code that uses raylib:: prefix
namespace raylib {
using ::IsMouseButtonPressed_Test;
using ::IsMouseButtonDown_Test;
using ::IsMouseButtonReleased_Test;
using ::IsMouseButtonUp_Test;
using ::GetCharPressed_Test;
using ::IsKeyPressed_Test;
using ::IsKeyDown_Test;
using ::GetMousePosition_Test;
using ::GetMouseWheelMove_Test;
}

#define IsMouseButtonPressed IsMouseButtonPressed_Test
#define IsMouseButtonDown IsMouseButtonDown_Test
#define IsMouseButtonReleased IsMouseButtonReleased_Test
#define IsMouseButtonUp IsMouseButtonUp_Test
#define GetCharPressed GetCharPressed_Test
#define IsKeyPressed IsKeyPressed_Test
#define IsKeyDown IsKeyDown_Test
#define GetMousePosition GetMousePosition_Test
#define GetMouseWheelMove GetMouseWheelMove_Test

#define AFTER_HOURS_USE_RAYLIB
#undef RectangleType
#undef Vector2Type
#undef TextureType
#undef FontType
#undef ColorType
#define RectangleType raylib::Rectangle
#define Vector2Type raylib::Vector2
#define TextureType raylib::Texture2D
#define FontType raylib::Font
#define ColorType raylib::Color
#define AFTER_HOURS_RECTANGLE_TYPE_DEFINED
#define AFTER_HOURS_VECTOR2_TYPE_DEFINED

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
