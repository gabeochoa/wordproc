# Metal Backend: RaylibBackend Struct

## Purpose

Move all raylib-specific function calls into a single struct that satisfies
the `GraphicsBackend` concept. This is a reorganization of existing code,
not new functionality.

## File Location

`vendor/afterhours/src/graphics/raylib_backend.h`

This file already exists but currently registers function pointers into
`BackendInterface`. We replace that with a plain struct of static functions.

## Implementation

```cpp
#pragma once

// Only compiled when raylib is the active backend
#ifdef AFTER_HOURS_USE_RAYLIB

#include "graphics_types.h"   // pulls in raylib headers via #ifdef
#include "../plugins/color.h" // afterhours::Color

namespace afterhours::graphics {

struct RaylibBackend {

    // ── Constants ──
    static constexpr unsigned int FLAG_WINDOW_RESIZABLE = raylib::FLAG_WINDOW_RESIZABLE;
    static constexpr int LOG_ERROR = raylib::LOG_ERROR;
    static constexpr int TEXTURE_FILTER_BILINEAR = raylib::TEXTURE_FILTER_BILINEAR;

    // ── Window lifecycle ──
    static void init_window(int w, int h, const char* title) {
        raylib::InitWindow(w, h, title);
    }
    static void close_window() { raylib::CloseWindow(); }
    static bool window_should_close() { return raylib::WindowShouldClose(); }
    static bool is_window_ready() { return raylib::IsWindowReady(); }
    static bool is_window_fullscreen() { return raylib::IsWindowFullscreen(); }
    static void toggle_fullscreen() { raylib::ToggleFullscreen(); }
    static void minimize_window() { raylib::MinimizeWindow(); }

    // ── Config ──
    static void set_config_flags(unsigned int flags) { raylib::SetConfigFlags(flags); }
    static void set_target_fps(int fps) { raylib::SetTargetFPS(fps); }
    static void set_exit_key(int key) { raylib::SetExitKey(key); }
    static void set_trace_log_level(int level) { raylib::SetTraceLogLevel(level); }

    // ── Frame ──
    static void begin_drawing() { raylib::BeginDrawing(); }
    static void end_drawing() { raylib::EndDrawing(); }
    static void clear_background(afterhours::Color c) { raylib::ClearBackground(c); }

    // ── Screen / timing ──
    static int get_screen_width() { return raylib::GetScreenWidth(); }
    static int get_screen_height() { return raylib::GetScreenHeight(); }
    static float get_frame_time() { return raylib::GetFrameTime(); }
    static float get_fps() { return static_cast<float>(raylib::GetFPS()); }
    static double get_time() { return raylib::GetTime(); }

    // ── Text measurement ──
    static int measure_text(const char* text, int fontSize) {
        return raylib::MeasureText(text, fontSize);
    }

    // ── Screenshots ──
    static void take_screenshot(const char* fileName) {
        raylib::TakeScreenshot(fileName);
    }

    // ── Input ──
    static bool is_key_pressed_repeat(int key) {
        return raylib::IsKeyPressedRepeat(key);
    }
};

static_assert(GraphicsBackend<RaylibBackend>,
              "RaylibBackend must satisfy GraphicsBackend concept");

}  // namespace afterhours::graphics

#endif  // AFTER_HOURS_USE_RAYLIB
```

## What happens to the existing code

| Existing code | Action |
|---|---|
| `raylib_backend.h` -- function pointer registration | Replace entirely with struct above |
| `raylib_windowed.h` -- windowed mode init | Keep for now, referenced by existing headless/windowed split. Can consolidate later. |
| `raylib_headless.h` -- headless mode init | Keep for now (used by e2e tests). |
| `graphics_backend.h` -- `BackendInterface` struct | Remove function pointers. Keep `get_backend()` only if headless mode still needs it. |

## Migration steps

1. Replace contents of `raylib_backend.h` with the struct above
2. Update `graphics.h` to use `using Backend = RaylibBackend;` instead of
   function-pointer dispatch
3. Add forwarding free functions in `graphics.h` (see `01-overview.md`)
4. Verify build -- all existing raylib behavior should be identical

## Headless mode consideration

The current `BackendInterface` function pointers exist so `raylib_windowed.h`
and `raylib_headless.h` can register different implementations at runtime.
For now we can keep that mechanism for the headless/windowed split within the
raylib backend. The `RaylibBackend` struct's `init_window` can internally
decide windowed vs headless based on `Config::display`. The concept doesn't
care about this internal detail.
