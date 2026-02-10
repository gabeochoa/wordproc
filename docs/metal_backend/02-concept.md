# Metal Backend: GraphicsBackend Concept

## Purpose

The `GraphicsBackend` concept defines every function a backend must provide.
A `static_assert` at compile time catches missing implementations immediately.
Only one backend is ever included -- no runtime dispatch, no function pointers.

## Concept Definition

The concept lives in `vendor/afterhours/src/graphics/graphics_concept.h`.
It replaces the current function-pointer `BackendInterface` with compile-time checks.

```cpp
#pragma once
#include <concepts>
#include <filesystem>
#include "graphics_types.h"

namespace afterhours::graphics {

struct Config;  // forward decl (already exists)

template <typename T>
concept GraphicsBackend = requires {
    // ── Window lifecycle ──
    { T::init_window(int{}, int{}, (const char*){}) } -> std::same_as<void>;
    { T::close_window() }                             -> std::same_as<void>;
    { T::window_should_close() }                      -> std::same_as<bool>;
    { T::is_window_ready() }                           -> std::same_as<bool>;
    { T::is_window_fullscreen() }                      -> std::same_as<bool>;
    { T::toggle_fullscreen() }                         -> std::same_as<void>;
    { T::minimize_window() }                            -> std::same_as<void>;

    // ── Config ──
    { T::set_config_flags(unsigned{}) }                -> std::same_as<void>;
    { T::set_target_fps(int{}) }                       -> std::same_as<void>;
    { T::set_exit_key(int{}) }                         -> std::same_as<void>;
    { T::set_trace_log_level(int{}) }                  -> std::same_as<void>;

    // ── Frame ──
    { T::begin_drawing() }                             -> std::same_as<void>;
    { T::end_drawing() }                               -> std::same_as<void>;
    { T::clear_background(afterhours::Color{}) }       -> std::same_as<void>;

    // ── Screen / timing ──
    { T::get_screen_width() }                          -> std::same_as<int>;
    { T::get_screen_height() }                         -> std::same_as<int>;
    { T::get_frame_time() }                            -> std::same_as<float>;
    { T::get_fps() }                                   -> std::same_as<float>;
    { T::get_time() }                                  -> std::same_as<double>;

    // ── Text measurement ──
    { T::measure_text((const char*){}, int{}) }        -> std::same_as<int>;

    // ── Screenshots ──
    { T::take_screenshot((const char*){}) }            -> std::same_as<void>;

    // ── Input ──
    { T::is_key_pressed_repeat(int{}) }                -> std::same_as<bool>;
};

}  // namespace afterhours::graphics
```

## What's NOT in the concept (yet)

These use backend-specific types (`raylib::Font`, `raylib::Texture2D`,
`raylib::RenderTexture2D`) that aren't abstracted yet:

- `load_font` / `set_texture_filter` -- uses `Font` and `Texture2D`
- `measure_text_ex` -- uses `Font` and returns `Vector2`
- `begin_texture_mode` / `end_texture_mode` -- uses `RenderTexture2D`

These stay as `#ifdef` blocks in `font_helper.h` and `drawing_helpers.h` for now.
When we abstract the Font/Texture types (see `06-types.md`), we can add them.

## What's NOT in the concept (handled elsewhere)

These are already abstracted in afterhours via `#ifdef` blocks in their
respective files. They don't need to be in the Backend struct:

- **Drawing primitives** (`draw_rectangle`, `draw_text`, etc.) -- `drawing_helpers.h`
- **Input** (`is_key_pressed`, `is_key_down`, mouse, gamepad) -- `input_system.h`
- **Font loading** (`load_font_from_file`, `measure_text`) -- `font_helper.h`
- **Color** -- `color.h`
- **Keycodes** -- `key_codes.h` (pure integer constants, no backend dependency)

When adding Metal support to those files, add `#elif defined(AFTER_HOURS_USE_METAL)`
branches. The concept only covers the *new* centralized functions.

## Constants

The backend struct also provides constants used by wordproc:

```cpp
// Inside each backend struct:
static constexpr unsigned int FLAG_WINDOW_RESIZABLE = /* backend value */;
static constexpr int LOG_ERROR = /* backend value */;
static constexpr int TEXTURE_FILTER_BILINEAR = /* backend value */;
```

These aren't checked by the concept (concepts can't check constexpr values),
but each backend must define them. The `static_assert` on the concept catches
the function requirements; constants are caught by normal compilation if missing.
