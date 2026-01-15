#pragma once

#include "external.h"

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
inline Vector2 operator*(float s, Vector2 a) { return Vector2Scale(a, s); }
inline Vector3 operator*(float s, Vector3 a) { return Vector3Scale(a, s); }
} // namespace raylib

#include <GLFW/glfw3.h>

#undef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400
#include <magic_enum/magic_enum.hpp>

#include "log.h"

#define AFTER_HOURS_INPUT_VALIDATION_ASSERT
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#define AFTER_HOURS_IMM_UI

#include <afterhours/ah.h>
#include <afterhours/src/developer.h>

namespace afterhours {
using vec2 = raylib::Vector2;
}
#include <afterhours/src/plugins/input_system.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <afterhours/src/plugins/window_manager.h>
#include <cassert>

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;
using raylib::Rectangle;

#include <afterhours/src/plugins/autolayout.h>
#include <afterhours/src/plugins/ui.h>

using afterhours::ui::Spacing;
using afterhours::ui::imm::DefaultSpacing;

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Owned by game.cpp
extern bool running;
extern raylib::RenderTexture2D mainRT;
extern raylib::RenderTexture2D screenRT;
extern raylib::Font uiFont;
