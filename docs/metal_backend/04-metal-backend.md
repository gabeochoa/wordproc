# Metal Backend: MetalBackend Stub

## Purpose

A minimal Metal backend that satisfies the `GraphicsBackend` concept.
Uses Sokol as the abstraction layer over Metal (handles window creation,
GPU context, and basic rendering).

## Why Sokol

- Single-header C libraries, easy to vendor
- `sokol_app.h` -- window/input (Metal-native on macOS, near-instant startup)
- `sokol_gfx.h` -- GPU rendering (Metal backend on macOS)
- `sokol_glue.h` -- ties app + gfx together
- Battle-tested, used in production (Floh/Andre Weissflog)
- No GLFW, no OpenGL context -- avoids the 1.3s macOS bottleneck entirely

## File Location

`vendor/afterhours/src/graphics/metal_backend.h`

## Stub Implementation

Start with a struct that compiles and does the minimum. Every function
either delegates to Sokol or logs a TODO. This lets us build and run
immediately, then fill in rendering piece by piece.

```cpp
#pragma once

#ifdef AFTER_HOURS_USE_METAL

// sokol headers (vendored or system)
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_time.h"

#include "../plugins/color.h"

namespace afterhours::graphics {

struct MetalBackend {

    // ── Constants ──
    // Match raylib values so afterhours::keys and existing keybinds just work.
    // Sokol uses different key values internally; map in input functions.
    static constexpr unsigned int FLAG_WINDOW_RESIZABLE = 0x00000004;
    static constexpr int LOG_ERROR = 5;
    static constexpr int TEXTURE_FILTER_BILINEAR = 1;

    // ── Internal state ──
    // Sokol manages its own state, but we track a few things.
    static inline double start_time_ = 0.0;
    static inline int target_fps_ = 60;

    // ── Window lifecycle ──
    static void init_window(int w, int h, const char* title) {
        // Sokol window creation happens in sokol_app's entry point.
        // For integration with afterhours, we may need to init sokol_gfx here:
        sg_desc desc = {};
        desc.environment = sglue_environment();
        sg_setup(&desc);
        stm_setup();
        start_time_ = stm_sec(stm_now());
        (void)w; (void)h; (void)title;
        // NOTE: width/height/title are passed via sapp_desc at program start.
        // This function confirms gfx is ready.
    }

    static void close_window() {
        sg_shutdown();
    }

    static bool window_should_close() {
        // sokol_app handles this via its event loop.
        // In a frame-callback model this always returns false;
        // the app exits when sapp requests quit.
        return false;
    }

    static bool is_window_ready() { return sg_isvalid(); }
    static bool is_window_fullscreen() { return sapp_is_fullscreen(); }
    static void toggle_fullscreen() { sapp_toggle_fullscreen(); }
    static void minimize_window() { /* TODO: sokol doesn't expose this */ }

    // ── Config ──
    static void set_config_flags(unsigned int) {
        // Handled via sapp_desc at startup
    }
    static void set_target_fps(int fps) { target_fps_ = fps; }
    static void set_exit_key(int) {
        // TODO: map to sokol key event handling
    }
    static void set_trace_log_level(int) {
        // TODO: configure sokol logging level
    }

    // ── Frame ──
    static void begin_drawing() {
        sg_pass pass = {};
        pass.swapchain = sglue_swapchain();
        // Default clear to black; clear_background overrides this
        pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass.action.colors[0].clear_value = {0, 0, 0, 1};
        sg_begin_pass(&pass);
    }

    static void end_drawing() {
        sg_end_pass();
        sg_commit();
    }

    static void clear_background(afterhours::Color c) {
        // NOTE: In Sokol, clear color is set at begin_pass time.
        // We may need to restructure so clear_background is called
        // before begin_pass, or re-issue the pass. For now, store
        // and apply on next begin_drawing.
        (void)c;
    }

    // ── Screen / timing ──
    static int get_screen_width() { return sapp_width(); }
    static int get_screen_height() { return sapp_height(); }
    static float get_frame_time() { return (float)sapp_frame_duration(); }
    static float get_fps() {
        float dt = get_frame_time();
        return dt > 0.0f ? 1.0f / dt : 0.0f;
    }
    static double get_time() {
        return stm_sec(stm_now()) - start_time_;
    }

    // ── Text measurement ──
    static int measure_text(const char*, int) {
        // TODO: needs font system (fontstash or custom)
        return 0;
    }

    // ── Screenshots ──
    static void take_screenshot(const char*) {
        // TODO: read pixels from Metal framebuffer and write PNG
    }

    // ── Input ──
    static bool is_key_pressed_repeat(int) {
        // TODO: track key repeat state from sokol_app events
        return false;
    }
};

static_assert(GraphicsBackend<MetalBackend>,
              "MetalBackend must satisfy GraphicsBackend concept");

}  // namespace afterhours::graphics

#endif  // AFTER_HOURS_USE_METAL
```

## Sokol integration notes

### Event loop model

Sokol uses a callback-based event loop (`sapp_desc.frame_cb`), not a
poll-based loop like raylib's `while (!WindowShouldClose())`. There are
two ways to integrate:

**Option A: Adapt sokol to poll-based** (recommended for minimal wordproc changes)
- Use `sokol_app.h` in "no-entry" mode (`SOKOL_NO_ENTRY`)
- Drive the loop from wordproc's existing `while` loop
- Call `sapp_frame()` manually each iteration

**Option B: Adapt wordproc to callback-based**
- Restructure main loop into `frame_cb` callback
- Bigger refactor, but more "sokol-native"

Recommend Option A to start. Wordproc's main loop stays as-is.

### Text rendering

Sokol doesn't include text rendering. Options:
- `fontstash` (sokol has `sokol_fontstash.h` integration)
- `stb_truetype.h` directly
- Custom SDF font renderer

This is the biggest piece of work for a fully functional Metal backend.
The stub returns 0 for `measure_text` so the app compiles but text won't
render until this is implemented.

### Input mapping

Sokol uses its own key enum (`SAPP_KEYCODE_*`). The values differ from
raylib/GLFW. We need a mapping table in the Metal backend's input functions
(and in the `#elif AFTER_HOURS_USE_METAL` branches of `input_system.h`).
The `afterhours::keys` constants stay the same -- the mapping happens
inside the backend.

## What this gives us

With just the stub, we can:
1. Compile with `AFTER_HOURS_USE_METAL` defined
2. Get a window open in <50ms (vs ~1300ms with raylib/OpenGL)
3. See a cleared background
4. Verify the abstraction layer works end-to-end

Then fill in: text rendering, drawing primitives, input, screenshots.
