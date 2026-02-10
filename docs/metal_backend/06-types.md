# Metal Backend: Type Abstractions

## Purpose

Several afterhours and wordproc APIs use raylib-specific types directly:
`raylib::Font`, `raylib::Texture2D`, `raylib::RenderTexture2D`. These need
backend-agnostic aliases so code using them compiles under Metal too.

## Current state

Afterhours already has some type aliases:

| Type | Alias | Defined in | Backend-agnostic? |
|------|-------|------------|-------------------|
| `raylib::Color` | `afterhours::Color` | `color.h` | Yes (4-byte struct `{r,g,b,a}`) |
| `raylib::Vector2` | `afterhours::vec2`, `Vector2Type` | `font_helper.h`, `external.h` | Partially -- `external.h` defines `#define Vector2Type raylib::Vector2` |
| `raylib::Rectangle` | `Rectangle`, `RectangleType` | `external.h`, `rl.h` | Partially -- `#define RectangleType raylib::Rectangle` |
| `raylib::Font` | `afterhours::Font` | `font_helper.h` | Yes (`#ifdef` with `FontType` fallback) |
| `raylib::Texture2D` | `TextureType` | `external.h` | Macro only |
| `raylib::RenderTexture2D` | `RenderTextureType` | `graphics_types.h` | Yes (placeholder struct when no raylib) |

## What wordproc uses

From `src/rl.h`:
```cpp
using vec2 = raylib::Vector2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;
using raylib::Rectangle;
extern raylib::RenderTexture2D mainRT;
extern raylib::RenderTexture2D screenRT;
extern raylib::Font uiFont;
```

From `src/ui/theme.h`:
```cpp
inline raylib::Font UI_FONT;
```

## Plan

### Phase 1: Use existing afterhours aliases (no afterhours changes)

Replace wordproc's direct `raylib::` type references with the aliases
afterhours already provides:

| wordproc code | Replace with |
|---|---|
| `raylib::Font UI_FONT` | `afterhours::Font UI_FONT` |
| `raylib::Font uiFont` | `afterhours::Font uiFont` |
| `raylib::Vector2` (in `rl.h`) | Already aliased as `vec2` -- no change |
| `raylib::Rectangle` (in `rl.h`) | Already `using raylib::Rectangle` -- replace with `RectangleType` |
| `raylib::RenderTexture2D mainRT` | `afterhours::graphics::RenderTextureType mainRT` |

This gets wordproc compiling without direct `raylib::` type references.
The underlying type is still `raylib::*` when `AFTER_HOURS_USE_RAYLIB` is defined,
but wordproc doesn't know or care.

### Phase 2: Ensure Metal types exist (afterhours change)

When `AFTER_HOURS_USE_METAL` is defined, the type aliases need to resolve to
something real:

```cpp
// In graphics_types.h or a new types header:
#ifdef AFTER_HOURS_USE_RAYLIB
using RenderTextureType = raylib::RenderTexture2D;
#elif defined(AFTER_HOURS_USE_METAL)
// Sokol doesn't have a direct equivalent -- we'd use sg_image + sg_pass
struct RenderTextureType {
    sg_image color;
    sg_image depth;
    sg_attachments attachments;
    int width, height;
};
#else
struct RenderTextureType {};
#endif
```

Similarly for Vector2/Rectangle if the current `#define` macros don't
have non-raylib paths:

```cpp
// In external.h or a new types header:
#ifdef AFTER_HOURS_USE_RAYLIB
#define Vector2Type raylib::Vector2
#define RectangleType raylib::Rectangle
#elif defined(AFTER_HOURS_USE_METAL)
struct Vector2Type { float x, y; };
struct RectangleType { float x, y, width, height; };
#endif
```

### Phase 3: Font abstraction (biggest piece)

`afterhours::Font` is aliased to `raylib::Font` under raylib, and `FontType`
otherwise. `FontType` is currently an empty struct placeholder.

For Metal, a real font type needs:
- Font atlas texture (`sg_image`)
- Glyph metrics (loaded via `stb_truetype.h` or `fontstash`)
- Character-to-glyph mapping

This is non-trivial and is the main blocker for text rendering under Metal.
Recommend implementing this after the window + basic rendering works.

Initial stub:
```cpp
#elif defined(AFTER_HOURS_USE_METAL)
struct FontType {
    // placeholder -- text rendering comes later
    bool valid = false;
};
using Font = FontType;
```

## Functions affected by type changes

These functions use backend-specific types and need `#elif` branches:

| Function | File | Types used |
|---|---|---|
| `LoadFont` | `font_helper.h` | `Font` |
| `MeasureTextEx` | `font_helper.h` | `Font`, `Vector2` |
| `SetTextureFilter` | `font_helper.h` | `Texture2D` |
| `BeginTextureMode` | `render_backend.h` / `graphics.h` | `RenderTexture2D` |
| `EndTextureMode` | `render_backend.h` / `graphics.h` | (no type, but paired with above) |
| `draw_text_ex` | `drawing_helpers.h` | `Font` |
| `draw_texture_npatch` | `drawing_helpers.h` | `Texture2D` |

These already have `#ifdef AFTER_HOURS_USE_RAYLIB` / `#else` stubs.
Adding `#elif AFTER_HOURS_USE_METAL` with Sokol implementations is
straightforward once the font system exists.

## Priority

1. **Now**: Replace `raylib::Font` -> `afterhours::Font` in wordproc (trivial)
2. **Now**: Replace `raylib::RenderTexture2D` -> `RenderTextureType` in wordproc
3. **Later**: Implement real Metal types when building out the Metal backend
4. **Later**: Font rendering system for Metal (biggest work item)
