# Metal Backend: Wordproc Migration

## Purpose

Once afterhours exposes the `GraphicsBackend`-based API, wordproc needs to
switch from its `render_backend.h` wrapper (and remaining direct `raylib::` calls)
to the afterhours API. This doc lists every file and what changes.

## Step 1: Delete `render_backend.h`

`src/render_backend.h` is the intermediate wrapper we built. It becomes
unnecessary once afterhours provides the same functions. Delete it.

All `render_backend::X(...)` calls become `afterhours::graphics::X(...)`.
All `render_backend::key::X` calls become `afterhours::keys::X`.

## Step 2: File-by-file changes

### Files using `render_backend::` (bulk rename)

| File | `render_backend::` calls | Replace with |
|------|--------------------------|--------------|
| `src/ecs/render_system.h` | `MeasureText`, `BeginDrawing`, `ClearBackground`, `EndDrawing`, `TakeScreenshot`, `GetScreenWidth`, `key::F1` | `afterhours::graphics::*`, `afterhours::keys::F1` |
| `src/ecs/input_system.h` | `key::TAB`, `key::LEFT_SHIFT`, `key::RIGHT_SHIFT`, `GetTime`, `GetScreenWidth`, `GetScreenHeight` | `afterhours::keys::*`, `afterhours::graphics::*` |
| `src/input/action_map.cpp` | `IsKeyPressedRepeat`, `key::*` (all keybinds) | `afterhours::graphics::is_key_pressed_repeat`, `afterhours::keys::*` |
| `src/main.cpp` | `WindowShouldClose`, `GetFrameTime`, `GetFPS` | `afterhours::graphics::*` |
| `src/preload.cpp` | `InitWindow`, `CloseWindow`, `SetTraceLogLevel`, `SetConfigFlags`, `SetTargetFPS`, `SetExitKey`, `LoadFont`, `SetTextureFilter`, `IsWindowReady` | `afterhours::graphics::*` (font functions stay `#ifdef` for now) |
| `src/settings.cpp` | `IsWindowFullscreen`, `ToggleFullscreen` | `afterhours::graphics::*` |
| `src/ui/theme.h` | `MeasureTextEx`, `MeasureText` | `afterhours::graphics::*` or `afterhours::measure_text` |
| `src/ecs/title_bar_system.h` | `MinimizeWindow` | `afterhours::graphics::minimize_window` |
| `src/extracted/status_notifications.h` | `GetTime` | `afterhours::graphics::get_time` |
| `src/ecs/screenshot_system.h` | `TakeScreenshot` | `afterhours::graphics::take_screenshot` |
| `src/testing/e2e_runner.cpp` | `TakeScreenshot` | `afterhours::graphics::take_screenshot` |

### Files with direct `raylib::` calls (not yet abstracted)

| File | Refs | Action |
|------|------|--------|
| `src/rl.h` (8 refs) | Type aliases: `vec2`, `vec3`, `vec4`, `Rectangle`, `RenderTexture2D`, `Font` | Keep for now. These are type aliases that come from afterhours `#ifdef`. See `06-types.md`. |
| `src/input_mapping.h` (20 refs) | `raylib::KEY_*` and `raylib::GAMEPAD_BUTTON_*` | Replace keys with `afterhours::keys::*`. Gamepad buttons need a similar abstraction in afterhours (new `afterhours::gamepad` constants). |
| `src/external.h` (20 refs) | Mouse/key functions wrapped for e2e test injection | These call `raylib::IsMouseButtonPressed_Real` etc. They're part of the test input injection layer. Needs `#elif AFTER_HOURS_USE_METAL` branches using Sokol equivalents. |
| `src/preload.cpp` (8 refs) | `raylib::SetTraceLogCallback`, `raylib::LOG_ERROR` | Trace callback is raylib-specific. Needs `#ifdef` or backend abstraction for log callbacks. |
| `src/util/logging.cpp` (2 refs) | `raylib::GetTime()` | Replace with `afterhours::graphics::get_time()` |
| `src/ui/theme.h` (1 ref) | `raylib::Font UI_FONT` | Replace with `afterhours::Font` (already aliased in `font_helper.h`) |
| `src/ecs/test_systems.h` (1 ref) | `raylib::TakeScreenshot` | Replace with `afterhours::graphics::take_screenshot` |
| `src/ui/input.h` (2 refs) | Comments only | No code change needed |

## Step 3: `input_mapping.h` keycodes + gamepad

Replace all `raylib::KEY_*` with `afterhours::keys::*`:

```
raylib::KEY_LEFT          -> afterhours::keys::LEFT
raylib::KEY_ESCAPE        -> afterhours::keys::ESCAPE
raylib::KEY_TAB           -> afterhours::keys::TAB
...etc
```

Gamepad buttons (`raylib::GAMEPAD_BUTTON_*`) don't have afterhours equivalents
yet. Options:
- **Quick**: Define `afterhours::gamepad::BUTTON_*` constants in a new
  `gamepad_codes.h` (same pattern as `key_codes.h`)
- **Deferred**: Keep `raylib::GAMEPAD_BUTTON_*` behind `#ifdef` in
  `input_mapping.h` since wordproc is keyboard-first and gamepad is low priority

Recommend deferred -- gamepad support is not blocking the Metal switch.

## Step 4: `external.h` test input layer

`external.h` wraps raylib calls for e2e test input injection. It calls things
like `raylib::IsKeyPressed_Real` which are raylib function pointers saved
before test injection patches them.

This needs backend-specific handling:
- Under `AFTER_HOURS_USE_RAYLIB`: current code stays
- Under `AFTER_HOURS_USE_METAL`: Sokol equivalents or skip test injection
  initially

Recommend: wrap with `#ifdef AFTER_HOURS_USE_RAYLIB` for now. E2E test
injection under Metal can come later.

## Step 5: Build flags

In the Makefile/CMake, change:
```
# Current:
-DAFTER_HOURS_USE_RAYLIB

# To switch to Metal:
-DAFTER_HOURS_USE_METAL
```

That's the only build change needed.

## Migration order

1. Update afterhours: concept, `RaylibBackend` struct, `graphics.h` forwarding
2. Build afterhours with raylib -- verify nothing breaks
3. In wordproc: replace `render_backend::` -> `afterhours::graphics::` / `afterhours::keys::`
4. Delete `src/render_backend.h`
5. Replace remaining `raylib::KEY_*` in `input_mapping.h` with `afterhours::keys::*`
6. Wrap `external.h` raylib calls with `#ifdef`
7. Replace `raylib::Font` / `raylib::GetTime` stragglers
8. Build and run -- should be identical behavior
9. Add `metal_backend.h` stub to afterhours
10. Try building with `-DAFTER_HOURS_USE_METAL` -- should compile with stubs
