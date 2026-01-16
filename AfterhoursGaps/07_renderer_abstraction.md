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

### Option 1: Interface-based (like wordproc's current approach)

```cpp
namespace afterhours {

struct IRenderer {
  virtual ~IRenderer() = default;
  
  // Frame management
  virtual void begin_frame() = 0;
  virtual void end_frame() = 0;
  virtual void clear(Color color) = 0;
  
  // Drawing primitives
  virtual void draw_rectangle(Rectangle rect, Color color) = 0;
  virtual void draw_rectangle_lines(Rectangle rect, float thickness, Color color) = 0;
  virtual void draw_line(Vector2 start, Vector2 end, Color color) = 0;
  virtual void draw_circle(Vector2 center, float radius, Color color) = 0;
  
  // Text
  virtual void draw_text(const Font& font, const std::string& text, 
                         Vector2 pos, float size, float spacing, Color color) = 0;
  virtual Vector2 measure_text(const Font& font, const std::string& text,
                               float size, float spacing) = 0;
  
  // Textures
  virtual void draw_texture(const Texture& tex, Rectangle src, Rectangle dst, 
                            Color tint) = 0;
  
  // Screen info
  virtual int screen_width() const = 0;
  virtual int screen_height() const = 0;
};

// Global renderer (set at startup)
IRenderer& get_renderer();
void set_renderer(std::unique_ptr<IRenderer> renderer);

// Built-in implementations
std::unique_ptr<IRenderer> create_raylib_renderer();
std::unique_ptr<IRenderer> create_headless_renderer(int width, int height);

} // namespace afterhours
```

### Option 2: Compile-time backend selection (current approach, improved)

Keep `#ifdef AFTER_HOURS_USE_RAYLIB` but provide complete implementations for:
- `AFTER_HOURS_USE_SDL2`
- `AFTER_HOURS_USE_HEADLESS` (for testing)

### Option 3: Render command buffer

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

## Notes
- Render abstraction adds overhead; may not be worth it for game-focused use cases
- Headless mode for E2E testing is the most practical immediate benefit
- Could be opt-in via compile flag to avoid overhead when not needed
- The current `#ifdef AFTER_HOURS_USE_RAYLIB` pattern is a starting point but needs 
  real alternative implementations to be useful

