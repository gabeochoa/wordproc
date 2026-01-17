# Renderer Abstraction Layer

## Working Implementation
See these files for a complete working example:
- `src/renderer/renderer_interface.h` - IRenderer abstract interface
- `src/renderer/renderer_interface.cpp` - Global renderer accessor
- `src/renderer/raylib_renderer.h` - Raylib backend implementation

## Problem
Afterhours is tightly coupled to raylib. There is no abstraction layer to support 
alternative rendering backends (SDL, OpenGL, Vulkan, software rendering, headless testing).

## Current Workaround
Custom `src/renderer/renderer_interface.h` defines an `IRenderer` interface:

```cpp
class IRenderer {
public:
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;
  virtual void clear(const Color& color) = 0;
  virtual void drawRect(const Rect& rect, const Color& color) = 0;
  virtual void drawRectLines(const Rect& rect, float thickness, const Color& color) = 0;
  virtual void drawLine(int x1, int y1, int x2, int y2, const Color& color) = 0;
  virtual void drawText(const std::string& text, int x, int y, int fontSize, const Color& color) = 0;
  virtual int measureText(const std::string& text, int fontSize) = 0;
  virtual int getScreenWidth() = 0;
  virtual int getScreenHeight() = 0;
};
```

The app provides a `RaylibRenderer` implementation and could swap in others.

## What Afterhours Currently Does
Throughout the codebase, direct raylib calls:
```cpp
raylib::DrawRectangle(...);
raylib::DrawText(...);
raylib::GetScreenWidth();
// etc.
```

These are wrapped in `#ifdef AFTER_HOURS_USE_RAYLIB` blocks but with no alternative 
implementations provided.

## Desired Behavior
A renderer abstraction that allows:
1. **Backend swapping** - Use SDL, OpenGL, Vulkan, or custom renderers
2. **Headless mode** - Run without a display for automated testing
3. **Render capture** - Capture frame output to image/buffer for testing
4. **Platform portability** - Easier porting to platforms without raylib

## Proposed API Sketch

### Recommended: Interface + Compile-time Selection + Free Functions

Combine interface abstraction with compile-time backend selection. The codebase just 
includes `graphics.h` and calls `DrawRect()`, `BeginFrame()`, etc. No `#ifdef`s scattered 
throughout—all backend selection is isolated to one header.

**User-facing API (what code includes):**

```cpp
// afterhours/src/graphics.h
// This is the ONLY header most code needs to include
#pragma once

#include "graphics_types.h"  // Color, Rectangle, Vector2, etc.

namespace afterhours {

// Simple free functions - no #ifdefs needed in calling code
void BeginFrame();
void EndFrame();
void Clear(Color color);

void DrawRectangle(Rectangle rect, Color color);
void DrawRectangleLines(Rectangle rect, float thickness, Color color);
void DrawLine(Vector2 start, Vector2 end, Color color);
void DrawCircle(Vector2 center, float radius, Color color);

void DrawText(const Font& font, const std::string& text, 
              Vector2 pos, float size, float spacing, Color color);
Vector2 MeasureText(const Font& font, const std::string& text,
                    float size, float spacing);

void DrawTexture(const Texture& tex, Rectangle src, Rectangle dst, Color tint);

int ScreenWidth();
int ScreenHeight();

// Initialization (call once at startup)
void InitGraphics(int width, int height, const char* title);
void ShutdownGraphics();

} // namespace afterhours
```

**Usage in game/app code:**

```cpp
#include <afterhours/graphics.h>

void render() {
    afterhours::BeginFrame();
    afterhours::Clear({30, 30, 30, 255});
    afterhours::DrawRectangle({100, 100, 200, 150}, {255, 0, 0, 255});
    afterhours::DrawText(font, "Hello", {100, 50}, 24, 1, WHITE);
    afterhours::EndFrame();
}
// No #ifdefs, no backend knowledge needed
```

**Internal: C++20 Concept (no virtual, no inheritance, zero overhead):**

```cpp
// afterhours/src/graphics_concept.h
#pragma once
#include <concepts>

namespace afterhours {

template<typename T>
concept GraphicsBackend = requires(T t, T const ct,
    int w, int h, const char* title,
    Color color, Rectangle rect, Vector2 v,
    float thickness, float size, float spacing,
    const Font& font, const std::string& text,
    const Texture& tex) 
{
  { t.init(w, h, title) } -> std::same_as<void>;
  { t.shutdown() } -> std::same_as<void>;
  { t.begin_frame() } -> std::same_as<void>;
  { t.end_frame() } -> std::same_as<void>;
  { t.clear(color) } -> std::same_as<void>;
  { t.draw_rectangle(rect, color) } -> std::same_as<void>;
  { t.draw_rectangle_lines(rect, thickness, color) } -> std::same_as<void>;
  { t.draw_line(v, v, color) } -> std::same_as<void>;
  { t.draw_circle(v, size, color) } -> std::same_as<void>;
  { t.draw_text(font, text, v, size, spacing, color) } -> std::same_as<void>;
  { t.measure_text(font, text, size, spacing) } -> std::same_as<Vector2>;
  { t.draw_texture(tex, rect, rect, color) } -> std::same_as<void>;
  { ct.screen_width() } -> std::same_as<int>;
  { ct.screen_height() } -> std::same_as<int>;
};

} // namespace afterhours
```

**Internal: Backend selection + global instance (header-only!):**

```cpp
// afterhours/src/graphics_backend.h
#pragma once

#include "graphics_concept.h"

#if defined(AFTERHOURS_USE_RAYLIB)
  #include "backends/raylib_backend.h"
  namespace afterhours { using Backend = RaylibBackend; }
#elif defined(AFTERHOURS_USE_SOKOL)
  #include "backends/sokol_backend.h"
  namespace afterhours { using Backend = SokolBackend; }
#elif defined(AFTERHOURS_USE_HEADLESS)
  #include "backends/headless_backend.h"
  namespace afterhours { using Backend = HeadlessBackend; }
#else
  #error "No graphics backend. Use -DAFTERHOURS_USE_RAYLIB, -DAFTERHOURS_USE_SOKOL, or -DAFTERHOURS_USE_HEADLESS"
#endif

namespace afterhours {

// Compile-time check that Backend satisfies the concept
static_assert(GraphicsBackend<Backend>, "Backend must satisfy GraphicsBackend concept");

namespace detail {
  inline Backend g_backend{};  // Concrete type, no heap allocation, no virtual
}

} // namespace afterhours
```

**Internal: Free functions (can be header-only with inline):**

```cpp
// afterhours/src/graphics.h (complete, header-only version)
#pragma once

#include "graphics_backend.h"

namespace afterhours {

inline void InitGraphics(int w, int h, const char* title) { detail::g_backend.init(w, h, title); }
inline void ShutdownGraphics() { detail::g_backend.shutdown(); }

inline void BeginFrame() { detail::g_backend.begin_frame(); }
inline void EndFrame() { detail::g_backend.end_frame(); }
inline void Clear(Color c) { detail::g_backend.clear(c); }

inline void DrawRectangle(Rectangle r, Color c) { detail::g_backend.draw_rectangle(r, c); }
inline void DrawRectangleLines(Rectangle r, float t, Color c) { detail::g_backend.draw_rectangle_lines(r, t, c); }
inline void DrawLine(Vector2 a, Vector2 b, Color c) { detail::g_backend.draw_line(a, b, c); }
inline void DrawCircle(Vector2 center, float radius, Color c) { detail::g_backend.draw_circle(center, radius, c); }

inline void DrawText(const Font& f, const std::string& t, Vector2 p, float sz, float sp, Color c) {
  detail::g_backend.draw_text(f, t, p, sz, sp, c);
}
inline Vector2 MeasureText(const Font& f, const std::string& t, float sz, float sp) {
  return detail::g_backend.measure_text(f, t, sz, sp);
}

inline void DrawTexture(const Texture& tex, Rectangle src, Rectangle dst, Color tint) {
  detail::g_backend.draw_texture(tex, src, dst, tint);
}

inline int ScreenWidth() { return detail::g_backend.screen_width(); }
inline int ScreenHeight() { return detail::g_backend.screen_height(); }

} // namespace afterhours
```

**Benefits of concepts approach:**
- **Zero overhead**: No virtual dispatch, no heap allocation, all calls are direct
- **Header-only**: No `.cpp` file needed, everything inlines
- **Better errors**: Concept violations give clear compile errors listing missing methods
- **No inheritance**: Backends are plain structs, no `override`, no base class
- **Simpler backends**: Just implement the methods, that's it

**Benefits of this approach:**
- **Clean API**: Game code just calls `DrawRect()`, no backend awareness
- **No scattered #ifdefs**: All conditional compilation in one file (`graphics_backend.h`)
- **Swappable at compile time**: `-DAFTERHOURS_USE_SOKOL` and rebuild
- **Testable**: Can inject mock/headless backend for unit tests
- **Extensible**: Add new backends by implementing `IGraphicsBackend`

### Build Flags

Use `-D` flags to select renderer at build time:

```bash
# Pick one:
-DAFTERHOURS_USE_RAYLIB     # Current raylib backend
-DAFTERHOURS_USE_SOKOL      # sokol_gfx (supports Metal/Vulkan/WebGPU/GL)
-DAFTERHOURS_USE_HEADLESS   # No-op for testing
```

When using sokol, you also specify the low-level graphics API:

```bash
# sokol_gfx sub-backend (pick one):
-DSOKOL_GLCORE33    # Desktop OpenGL 3.3
-DSOKOL_METAL       # macOS/iOS Metal
-DSOKOL_D3D11       # Windows Direct3D 11
-DSOKOL_WGPU        # WebGPU (native or browser)
-DSOKOL_GLES3       # Mobile / Emscripten OpenGL ES3
```

**Why sokol_gfx?**
- Single C header, easy to vendor
- Handles Metal/Vulkan/D3D11/WebGPU/OpenGL behind one API
- Actively maintained, used in production
- Works with Emscripten for web builds
- You implement against sokol once, get all backends free

**Example Makefile integration:**

```makefile
# Backend selection
ifeq ($(RENDERER),raylib)
  CXXFLAGS += -DAFTERHOURS_USE_RAYLIB
  LIBS += -lraylib
else ifeq ($(RENDERER),sokol)
  CXXFLAGS += -DAFTERHOURS_USE_SOKOL
  # Auto-detect platform for sokol sub-backend
  ifeq ($(shell uname),Darwin)
    CXXFLAGS += -DSOKOL_METAL
    LIBS += -framework Metal -framework MetalKit -framework Cocoa
  else ifeq ($(OS),Windows_NT)
    CXXFLAGS += -DSOKOL_D3D11
    LIBS += -ld3d11 -ldxgi
  else
    CXXFLAGS += -DSOKOL_GLCORE33
    LIBS += -lGL
  endif
else ifeq ($(RENDERER),headless)
  CXXFLAGS += -DAFTERHOURS_USE_HEADLESS
else ifeq ($(RENDERER),wasm)
  CXXFLAGS += -DAFTERHOURS_USE_SOKOL -DSOKOL_GLES3
  # Emscripten handles GL linkage
endif

# Usage: make RENDERER=sokol
# Usage: make RENDERER=raylib
# Usage: make RENDERER=wasm  (with emcc)
```

**Sokol backend implementation sketch:**

```cpp
// backends/sokol_backend.h
#pragma once

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

namespace afterhours {

// No inheritance needed - just satisfy the GraphicsBackend concept
struct SokolBackend {
  sg_pass_action pass_action_{};
  sg_pipeline quad_pipeline_{};
  sg_bindings quad_bindings_{};
  std::vector<QuadVertex> quad_batch_;

  void init(int width, int height, const char* title) {
    sg_desc desc{};
    desc.context = sapp_sgcontext();
    sg_setup(&desc);
    setup_quad_pipeline();
  }
  
  void shutdown() { sg_shutdown(); }
  
  void begin_frame() {
    sg_begin_default_pass(&pass_action_, sapp_width(), sapp_height());
  }
  
  void end_frame() {
    flush_batch();
    sg_end_pass();
    sg_commit();
  }
  
  void draw_rectangle(Rectangle rect, Color color) {
    quad_batch_.push_back({rect.x, rect.y, ...});
  }
  
  void draw_rectangle_lines(Rectangle rect, float thickness, Color color) { /*...*/ }
  void draw_line(Vector2 a, Vector2 b, Color color) { /*...*/ }
  void draw_circle(Vector2 center, float radius, Color color) { /*...*/ }
  void draw_text(const Font& f, const std::string& t, Vector2 p, float sz, float sp, Color c) { /*...*/ }
  Vector2 measure_text(const Font& f, const std::string& t, float sz, float sp) { return {}; }
  void draw_texture(const Texture& tex, Rectangle src, Rectangle dst, Color tint) { /*...*/ }
  
  void clear(Color color) {
    pass_action_.colors[0] = {
      .action = SG_ACTION_CLEAR,
      .value = { color.r/255.f, color.g/255.f, color.b/255.f, color.a/255.f }
    };
  }
  
  int screen_width() const { return sapp_width(); }
  int screen_height() const { return sapp_height(); }
  
private:
  struct QuadVertex { float x, y, u, v; uint32_t color; };
  
  void flush_batch() {
    if (quad_batch_.empty()) return;
    sg_update_buffer(quad_bindings_.vertex_buffers[0], ...);
    sg_apply_pipeline(quad_pipeline_);
    sg_apply_bindings(&quad_bindings_);
    sg_draw(0, quad_batch_.size() * 6, 1);
    quad_batch_.clear();
  }
};

} // namespace afterhours
```

### Alternative: Render command buffer (for advanced use cases)

```cpp
namespace afterhours {

// Deferred rendering - collect commands, execute later
struct RenderCommand {
  enum Type { DrawRect, DrawText, DrawLine, ... };
  Type type;
  // union of command data
};

struct RenderCommandBuffer {
  void draw_rectangle(Rectangle rect, Color color);
  void draw_text(...);
  // etc.
  
  std::vector<RenderCommand> commands;
  
  // Execute on a specific backend
  void execute(IRenderer& renderer);
  
  // Or serialize for testing
  std::string to_string() const;
};

} // namespace afterhours
```

## Supported Backends Summary

| Define | Backend | Platforms |
|--------|---------|-----------|
| `AFTERHOURS_USE_RAYLIB` | raylib | Desktop (current) |
| `AFTERHOURS_USE_SOKOL` + `SOKOL_METAL` | Metal | macOS, iOS |
| `AFTERHOURS_USE_SOKOL` + `SOKOL_D3D11` | Direct3D 11 | Windows |
| `AFTERHOURS_USE_SOKOL` + `SOKOL_GLCORE33` | OpenGL 3.3 | Linux, macOS, Windows |
| `AFTERHOURS_USE_SOKOL` + `SOKOL_GLES3` | OpenGL ES 3 | Mobile, Web (Emscripten) |
| `AFTERHOURS_USE_SOKOL` + `SOKOL_WGPU` | WebGPU | Browser (native WebGPU), Dawn |
| `AFTERHOURS_USE_HEADLESS` | No-op | Any (for testing) |

## Notes
- **Zero overhead** with concepts: no virtual dispatch, no heap allocation, all calls inline
- **Header-only**: The entire abstraction can live in headers, no `.cpp` needed
- Headless mode enables automated visual regression testing
- The single `graphics.h` include keeps game/app code clean
- All `#ifdef` logic isolated to `graphics_backend.h`—nowhere else
- Requires C++20 (`-std=c++20`)

## Resources
- https://github.com/floooh/sokol
- https://github.com/floooh/sokol-samples (example code)
- https://github.com/nicbarker/sokol-raylib (raylib-like API on top of sokol)


