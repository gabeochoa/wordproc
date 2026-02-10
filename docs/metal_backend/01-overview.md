# Metal Backend: Overview

## Problem

`InitWindow` takes ~1.3s on macOS due to OpenGL context creation via GLFW/raylib.
This is an OS-level bottleneck -- not optimizable in application code.
Metal window creation is near-instant on macOS.

## Goal

Swap raylib for a Metal-based backend (via Sokol) by changing a single `#define`.
No runtime backend switching. One backend compiles at a time.

## Architecture

```
afterhours/
  src/
    core/
      key_codes.h              # already backend-agnostic (integer constants)
    graphics/
      graphics_concept.h       # C++20 concept: what every backend must provide
      graphics_types.h         # #ifdef type aliases (RenderTextureType, etc.)
      graphics.h               # public API: using Backend = ...; free functions
      raylib_backend.h         # struct RaylibBackend -- static functions wrapping raylib
      metal_backend.h          # struct MetalBackend  -- static functions wrapping Sokol/Metal
    plugins/
      input_system.h           # already has #ifdef AFTER_HOURS_USE_RAYLIB / #else
      color.h                  # already backend-agnostic
    drawing_helpers.h          # already has #ifdef AFTER_HOURS_USE_RAYLIB / #else
    font_helper.h              # already backend-agnostic types
```

## How It Works

1. `graphics_concept.h` defines `concept GraphicsBackend<T>` requiring every
   function a backend must implement (window, frame, screen, text, input, etc.)

2. Each backend is a struct of static functions in its own file:
   - `RaylibBackend` in `raylib_backend.h`
   - `MetalBackend` in `metal_backend.h`

3. `graphics.h` selects the backend at compile time:
   ```cpp
   #ifdef AFTER_HOURS_USE_RAYLIB
   #include "raylib_backend.h"
   using Backend = RaylibBackend;
   #elif defined(AFTER_HOURS_USE_METAL)
   #include "metal_backend.h"
   using Backend = MetalBackend;
   #endif
   static_assert(GraphicsBackend<Backend>);
   ```

4. Public free functions forward to `Backend::`:
   ```cpp
   inline void init_window(int w, int h, const char* title) {
       Backend::init_window(w, h, title);
   }
   ```

5. Wordproc calls `afterhours::graphics::init_window(...)` etc.
   The backend is invisible to application code.

## What Changes

| Layer | Change |
|-------|--------|
| `graphics_concept.h` | Expand concept with all required functions |
| `graphics_backend.h` | Remove function-pointer `BackendInterface` (replaced by concept) |
| `graphics.h` | Replace function-pointer dispatch with `Backend::` forwarding |
| `raylib_backend.h` | Move existing raylib calls into `RaylibBackend` struct |
| `metal_backend.h` | New file: stub struct satisfying concept |
| `drawing_helpers.h` | No change (already `#ifdef`-based) |
| `input_system.h` | No change (already `#ifdef`-based) |
| wordproc `render_backend.h` | Delete -- replaced by afterhours API |
| wordproc source files | `render_backend::X` -> `afterhours::graphics::X` |
| wordproc source files | `render_backend::key::X` -> `afterhours::keys::X` |
| wordproc `input_mapping.h` | `raylib::KEY_X` -> `afterhours::keys::X` |

## What Stays the Same

- `key_codes.h` -- already pure integer constants, no backend dependency
- `Color`, `Rectangle`, `Vector2` type aliases -- already in `external.h`/`color.h`
- `afterhours::draw_*` functions -- already `#ifdef`-gated in `drawing_helpers.h`
- `afterhours::input::is_key_pressed` etc. -- already `#ifdef`-gated
- ECS architecture, systems, components -- all backend-agnostic
