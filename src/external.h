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
inline Vector2 GetMousePosition_Real() { return GetMousePosition(); }

} // namespace raylib

#include "testing/test_input_fwd.h"

namespace raylib {
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
inline Vector2 GetMousePosition_Test() {
  auto pos = test_input::get_mouse_position_fwd();
  return Vector2{pos.x, pos.y};
}
} // namespace raylib

#define IsMouseButtonPressed IsMouseButtonPressed_Test
#define IsMouseButtonDown IsMouseButtonDown_Test
#define IsMouseButtonReleased IsMouseButtonReleased_Test
#define IsMouseButtonUp IsMouseButtonUp_Test
#define GetCharPressed GetCharPressed_Test
#define IsKeyPressed IsKeyPressed_Test
#define GetMousePosition GetMousePosition_Test

#define AFTER_HOURS_USE_RAYLIB
#undef RectangleType
#undef Vector2Type
#undef TextureType
#undef FontType
#define RectangleType raylib::Rectangle
#define Vector2Type raylib::Vector2
#define TextureType raylib::Texture2D
#define FontType raylib::Font
#define AFTER_HOURS_RECTANGLE_TYPE_DEFINED
#define AFTER_HOURS_VECTOR2_TYPE_DEFINED

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
