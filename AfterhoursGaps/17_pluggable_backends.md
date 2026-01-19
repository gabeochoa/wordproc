# Gap 17: Pluggable Backend System

## Problem

Afterhours currently has hardcoded implementations for system integrations like clipboard, file dialogs, and potentially other OS-level features. When applications need to:

1. **Mock for testing** - Replace system clipboard with in-memory storage for isolated E2E tests
2. **Support multiple platforms** - Use different implementations on different OSes
3. **Enable headless mode** - Run without a display for CI/server environments
4. **Inject custom behavior** - Add logging, validation, or custom handling

...they must work around the library by wrapping all calls at the application layer.

## Current Afterhours Implementation

```cpp
// vendor/afterhours/src/plugins/clipboard.h
namespace afterhours::clipboard {

#ifdef AFTER_HOURS_USE_RAYLIB
inline void set_text(std::string_view text) {
    std::string str(text);
    raylib::SetClipboardText(str.c_str());  // Hardcoded raylib call
}

inline std::string get_text() {
    const char *text = raylib::GetClipboardText();
    return text ? std::string(text) : "";
}
#else
// Empty fallback - no way to inject custom implementation
inline void set_text(std::string_view) {}
inline std::string get_text() { return ""; }
#endif

}
```

## Required Workaround

Applications must create their own wrapper layer that duplicates the API:

```cpp
// src/util/clipboard.h (application workaround)
namespace app::clipboard {

namespace detail {
    inline std::string test_clipboard_text;
    inline bool test_mode_enabled = false;
}

inline void enable_test_mode() { detail::test_mode_enabled = true; }

inline void set_text(std::string_view text) {
    if (detail::test_mode_enabled) {
        detail::test_clipboard_text = std::string(text);
        return;
    }
    afterhours::clipboard::set_text(text);
}

inline std::string get_text() {
    if (detail::test_mode_enabled) {
        return detail::test_clipboard_text;
    }
    return afterhours::clipboard::get_text();
}

}
```

Then all code must use `app::clipboard::` instead of `afterhours::clipboard::`.

---

## Proposed Solution: Sokol as Alternative Backend

We propose using **Sokol** (https://github.com/floooh/sokol) as the primary alternative to Raylib. Sokol is a collection of single-file, cross-platform C libraries that provide a more modular and lightweight approach to graphics, windowing, and system integration.

### Why Sokol?

| Aspect | Raylib | Sokol |
|--------|--------|-------|
| **Architecture** | Monolithic | Modular single-file headers |
| **Graphics API** | OpenGL-focused | GL/GLES3/WebGL2 + Metal + D3D11 + WebGPU |
| **WebAssembly** | Supported | First-class support with Emscripten |
| **Binary size** | Larger | Minimal, pay-for-what-you-use |
| **Headless mode** | Limited | Built-in dummy backends |
| **Header-only** | No | Yes (STB-style) |
| **Dependencies** | Multiple | Zero (except platform libs) |

### Sokol Library Overview

#### Core Libraries (What We Need)

| Library | Purpose | Replaces in Raylib |
|---------|---------|-------------------|
| `sokol_gfx.h` | 3D-API wrapper (GL/GLES3/WebGL2 + Metal + D3D11 + WebGPU) | `rlgl.h`, core rendering |
| `sokol_app.h` | App framework (entry + window + 3D-context + input) | `core.h` window/input |
| `sokol_time.h` | High-resolution time measurement | `GetTime()`, `GetFrameTime()` |
| `sokol_audio.h` | Minimal buffer-streaming audio playback | `raudio.h` |
| `sokol_fetch.h` | Async data streaming (HTTP + filesystem) | File loading helpers |
| `sokol_args.h` | Unified cmdline/URL arg parser | N/A |
| `sokol_log.h` | Standard logging callback | `TraceLog()` |

#### Utility Libraries (Optional)

| Library | Purpose | Use Case |
|---------|---------|----------|
| `sokol_imgui.h` | Dear ImGui rendering backend | Debug UI integration |
| `sokol_nuklear.h` | Nuklear rendering backend | Alternative UI |
| `sokol_gl.h` | OpenGL 1.x immediate-mode API | Simple 2D drawing |
| `sokol_fontstash.h` | Font rendering via fontstash | Text rendering |
| `sokol_gfx_imgui.h` | Debug UI for sokol_gfx | Graphics debugging |
| `sokol_debugtext.h` | Vintage font text renderer | Debug overlays |
| `sokol_memtrack.h` | Memory allocation tracking | Debug/profiling |
| `sokol_shape.h` | Procedural shape generation | 3D primitives |
| `sokol_color.h` | X11 color constants | Color utilities |
| `sokol_spine.h` | Spine runtime wrapper | 2D animation |

---

## Backend Interface Design

### Core Backend Abstraction

```cpp
// afterhours/src/backends/backend.h
namespace afterhours::backend {

// Unified backend interface for all system integrations
struct IBackend {
    virtual ~IBackend() = default;
    
    // Lifecycle
    virtual bool init(const BackendConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool is_headless() const = 0;
    
    // Window
    virtual void* get_native_window_handle() = 0;
    virtual Vec2i get_window_size() = 0;
    virtual void set_window_size(Vec2i size) = 0;
    virtual void set_window_title(std::string_view title) = 0;
    virtual bool should_close() = 0;
    virtual void poll_events() = 0;
    
    // Rendering
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void clear(Color color) = 0;
    
    // Input
    virtual bool is_key_pressed(Key key) = 0;
    virtual bool is_key_down(Key key) = 0;
    virtual bool is_key_released(Key key) = 0;
    virtual Vec2f get_mouse_position() = 0;
    virtual bool is_mouse_button_pressed(MouseButton btn) = 0;
    virtual float get_mouse_wheel() = 0;
    
    // Clipboard
    virtual void set_clipboard_text(std::string_view text) = 0;
    virtual std::string get_clipboard_text() = 0;
    virtual bool has_clipboard_text() = 0;
    
    // Cursor
    virtual void set_cursor(CursorType cursor) = 0;
    virtual void show_cursor(bool visible) = 0;
    
    // Time
    virtual double get_time() = 0;
    virtual float get_delta_time() = 0;
    
    // Drawing primitives (for immediate mode)
    virtual void draw_rectangle(Rect rect, Color color) = 0;
    virtual void draw_rectangle_outline(Rect rect, Color color, float thickness) = 0;
    virtual void draw_text(std::string_view text, Vec2f pos, float size, Color color) = 0;
    virtual void draw_line(Vec2f start, Vec2f end, Color color, float thickness) = 0;
    virtual void draw_circle(Vec2f center, float radius, Color color) = 0;
    
    // Textures
    virtual TextureHandle load_texture(const uint8_t* data, int width, int height, PixelFormat format) = 0;
    virtual void unload_texture(TextureHandle handle) = 0;
    virtual void draw_texture(TextureHandle handle, Rect src, Rect dst, Color tint) = 0;
    
    // Fonts
    virtual FontHandle load_font(const char* path, int size) = 0;
    virtual void unload_font(FontHandle handle) = 0;
    virtual Vec2f measure_text(FontHandle font, std::string_view text, float size) = 0;
    virtual void draw_text_ex(FontHandle font, std::string_view text, Vec2f pos, float size, Color color) = 0;
};

// Backend configuration
struct BackendConfig {
    int window_width = 800;
    int window_height = 600;
    std::string window_title = "Afterhours";
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    bool headless = false;  // For testing/CI
    int target_fps = 60;
    int msaa_samples = 4;
};

}
```

### Sokol Backend Implementation

```cpp
// afterhours/src/backends/sokol_backend.h
#pragma once

#define SOKOL_IMPL
#define SOKOL_GLCORE33  // or SOKOL_METAL, SOKOL_D3D11, SOKOL_WGPU
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_gl.h"
#include "sokol_fontstash.h"
#include "sokol_log.h"

namespace afterhours::backend {

class SokolBackend : public IBackend {
private:
    BackendConfig config_;
    sg_pass_action pass_action_;
    sgl_pipeline pipeline_;
    FONScontext* fons_ctx_ = nullptr;
    uint64_t last_time_ = 0;
    float delta_time_ = 0.0f;
    
    // Input state tracking
    struct InputState {
        bool keys_pressed[512] = {};
        bool keys_down[512] = {};
        bool keys_released[512] = {};
        bool mouse_pressed[8] = {};
        bool mouse_down[8] = {};
        Vec2f mouse_pos = {0, 0};
        float mouse_wheel = 0;
    } input_;
    
    // Clipboard buffer (sokol_app handles clipboard via callbacks)
    std::string clipboard_buffer_;

public:
    bool init(const BackendConfig& cfg) override {
        config_ = cfg;
        
        // sokol_app handles window creation - this is called from sapp_desc
        // Initialize graphics
        sg_desc gfx_desc = {};
        gfx_desc.environment = sglue_environment();
        gfx_desc.logger.func = slog_func;
        sg_setup(&gfx_desc);
        
        // Initialize sokol_gl for immediate mode drawing
        sgl_desc_t gl_desc = {};
        gl_desc.logger.func = slog_func;
        sgl_setup(&gl_desc);
        
        // Initialize time
        stm_setup();
        last_time_ = stm_now();
        
        // Setup default pass action (clear color)
        pass_action_.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action_.colors[0].clear_value = {0.1f, 0.1f, 0.1f, 1.0f};
        
        // Initialize fontstash
        init_fonts();
        
        return true;
    }
    
    void shutdown() override {
        if (fons_ctx_) {
            sfons_destroy(fons_ctx_);
        }
        sgl_shutdown();
        sg_shutdown();
    }
    
    bool is_headless() const override {
        return config_.headless;
    }
    
    // Window management (delegated to sokol_app)
    void* get_native_window_handle() override {
        // Platform-specific window handle retrieval
        #if defined(_WIN32)
            return (void*)sapp_win32_get_hwnd();
        #elif defined(__APPLE__)
            return sapp_macos_get_window();
        #elif defined(__linux__)
            return (void*)sapp_linux_get_display();
        #else
            return nullptr;
        #endif
    }
    
    Vec2i get_window_size() override {
        return {sapp_width(), sapp_height()};
    }
    
    void set_window_size(Vec2i size) override {
        // sokol_app doesn't support runtime resize - handle via config
    }
    
    void set_window_title(std::string_view title) override {
        sapp_set_window_title(std::string(title).c_str());
    }
    
    bool should_close() override {
        return sapp_quit_requested();
    }
    
    void poll_events() override {
        // sokol_app handles event polling internally
        // Reset per-frame input states
        std::memset(input_.keys_pressed, 0, sizeof(input_.keys_pressed));
        std::memset(input_.keys_released, 0, sizeof(input_.keys_released));
        std::memset(input_.mouse_pressed, 0, sizeof(input_.mouse_pressed));
        input_.mouse_wheel = 0;
    }
    
    // Frame management
    void begin_frame() override {
        uint64_t now = stm_now();
        delta_time_ = (float)stm_sec(stm_diff(now, last_time_));
        last_time_ = now;
        
        sg_begin_pass({
            .action = pass_action_,
            .swapchain = sglue_swapchain()
        });
        
        sgl_defaults();
        sgl_matrix_mode_projection();
        sgl_ortho(0.0f, (float)sapp_width(), (float)sapp_height(), 0.0f, -1.0f, 1.0f);
    }
    
    void end_frame() override {
        sgl_draw();
        sg_end_pass();
        sg_commit();
    }
    
    void clear(Color color) override {
        pass_action_.colors[0].clear_value = {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f
        };
    }
    
    // Input handling
    bool is_key_pressed(Key key) override {
        return input_.keys_pressed[sokol_key_code(key)];
    }
    
    bool is_key_down(Key key) override {
        return input_.keys_down[sokol_key_code(key)];
    }
    
    bool is_key_released(Key key) override {
        return input_.keys_released[sokol_key_code(key)];
    }
    
    Vec2f get_mouse_position() override {
        return input_.mouse_pos;
    }
    
    bool is_mouse_button_pressed(MouseButton btn) override {
        return input_.mouse_pressed[static_cast<int>(btn)];
    }
    
    float get_mouse_wheel() override {
        return input_.mouse_wheel;
    }
    
    // Clipboard
    void set_clipboard_text(std::string_view text) override {
        clipboard_buffer_ = std::string(text);
        sapp_set_clipboard_string(clipboard_buffer_.c_str());
    }
    
    std::string get_clipboard_text() override {
        const char* text = sapp_get_clipboard_string();
        return text ? std::string(text) : "";
    }
    
    bool has_clipboard_text() override {
        const char* text = sapp_get_clipboard_string();
        return text && text[0] != '\0';
    }
    
    // Cursor
    void set_cursor(CursorType cursor) override {
        sapp_mouse_cursor sokol_cursor;
        switch (cursor) {
            case CursorType::Arrow:      sokol_cursor = SAPP_MOUSECURSOR_ARROW; break;
            case CursorType::IBeam:      sokol_cursor = SAPP_MOUSECURSOR_IBEAM; break;
            case CursorType::Crosshair:  sokol_cursor = SAPP_MOUSECURSOR_CROSSHAIR; break;
            case CursorType::Hand:       sokol_cursor = SAPP_MOUSECURSOR_POINTING_HAND; break;
            case CursorType::ResizeEW:   sokol_cursor = SAPP_MOUSECURSOR_RESIZE_EW; break;
            case CursorType::ResizeNS:   sokol_cursor = SAPP_MOUSECURSOR_RESIZE_NS; break;
            case CursorType::ResizeNWSE: sokol_cursor = SAPP_MOUSECURSOR_RESIZE_NWSE; break;
            case CursorType::ResizeNESW: sokol_cursor = SAPP_MOUSECURSOR_RESIZE_NESW; break;
            case CursorType::ResizeAll:  sokol_cursor = SAPP_MOUSECURSOR_RESIZE_ALL; break;
            case CursorType::NotAllowed: sokol_cursor = SAPP_MOUSECURSOR_NOT_ALLOWED; break;
            default:                     sokol_cursor = SAPP_MOUSECURSOR_DEFAULT; break;
        }
        sapp_set_mouse_cursor(sokol_cursor);
    }
    
    void show_cursor(bool visible) override {
        sapp_show_mouse(visible);
    }
    
    // Time
    double get_time() override {
        return stm_sec(stm_now());
    }
    
    float get_delta_time() override {
        return delta_time_;
    }
    
    // Drawing (via sokol_gl)
    void draw_rectangle(Rect rect, Color color) override {
        sgl_begin_quads();
        set_color(color);
        sgl_v2f(rect.x, rect.y);
        sgl_v2f(rect.x + rect.width, rect.y);
        sgl_v2f(rect.x + rect.width, rect.y + rect.height);
        sgl_v2f(rect.x, rect.y + rect.height);
        sgl_end();
    }
    
    void draw_rectangle_outline(Rect rect, Color color, float thickness) override {
        sgl_begin_lines();
        set_color(color);
        // Top
        sgl_v2f(rect.x, rect.y);
        sgl_v2f(rect.x + rect.width, rect.y);
        // Right
        sgl_v2f(rect.x + rect.width, rect.y);
        sgl_v2f(rect.x + rect.width, rect.y + rect.height);
        // Bottom
        sgl_v2f(rect.x + rect.width, rect.y + rect.height);
        sgl_v2f(rect.x, rect.y + rect.height);
        // Left
        sgl_v2f(rect.x, rect.y + rect.height);
        sgl_v2f(rect.x, rect.y);
        sgl_end();
    }
    
    void draw_line(Vec2f start, Vec2f end, Color color, float thickness) override {
        sgl_begin_lines();
        set_color(color);
        sgl_v2f(start.x, start.y);
        sgl_v2f(end.x, end.y);
        sgl_end();
    }
    
    void draw_circle(Vec2f center, float radius, Color color) override {
        const int segments = 32;
        sgl_begin_triangle_strip();
        set_color(color);
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            sgl_v2f(center.x, center.y);
            sgl_v2f(center.x + cosf(angle) * radius, 
                    center.y + sinf(angle) * radius);
        }
        sgl_end();
    }

private:
    void set_color(Color c) {
        sgl_c4b(c.r, c.g, c.b, c.a);
    }
    
    void init_fonts() {
        // Initialize fontstash context for text rendering
        fons_ctx_ = sfons_create(&(sfons_desc_t){
            .width = 512,
            .height = 512,
        });
    }
    
    // Event callback (called by sokol_app)
public:
    void handle_event(const sapp_event* ev) {
        switch (ev->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
                input_.keys_down[ev->key_code] = true;
                input_.keys_pressed[ev->key_code] = true;
                break;
            case SAPP_EVENTTYPE_KEY_UP:
                input_.keys_down[ev->key_code] = false;
                input_.keys_released[ev->key_code] = true;
                break;
            case SAPP_EVENTTYPE_MOUSE_DOWN:
                input_.mouse_down[ev->mouse_button] = true;
                input_.mouse_pressed[ev->mouse_button] = true;
                break;
            case SAPP_EVENTTYPE_MOUSE_UP:
                input_.mouse_down[ev->mouse_button] = false;
                break;
            case SAPP_EVENTTYPE_MOUSE_MOVE:
                input_.mouse_pos = {ev->mouse_x, ev->mouse_y};
                break;
            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                input_.mouse_wheel = ev->scroll_y;
                break;
            default:
                break;
        }
    }
};

}
```

### Raylib Backend (Existing)

```cpp
// afterhours/src/backends/raylib_backend.h
#pragma once
#include "raylib.h"

namespace afterhours::backend {

class RaylibBackend : public IBackend {
    // ... existing raylib implementation wrapped in IBackend interface
    
    bool init(const BackendConfig& config) override {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
        InitWindow(config.window_width, config.window_height, 
                   config.window_title.c_str());
        SetTargetFPS(config.target_fps);
        return true;
    }
    
    void set_clipboard_text(std::string_view text) override {
        SetClipboardText(std::string(text).c_str());
    }
    
    std::string get_clipboard_text() override {
        const char* t = GetClipboardText();
        return t ? std::string(t) : "";
    }
    
    // ... rest of implementation
};

}
```

### Headless/Test Backend

```cpp
// afterhours/src/backends/headless_backend.h
#pragma once

namespace afterhours::backend {

class HeadlessBackend : public IBackend {
private:
    BackendConfig config_;
    double simulated_time_ = 0.0;
    float delta_time_ = 1.0f / 60.0f;  // Simulated 60fps
    std::string clipboard_;
    
    // Virtual screen buffer for screenshot testing
    std::vector<uint8_t> framebuffer_;
    int fb_width_ = 0, fb_height_ = 0;
    
    // Input queue for scripted testing
    struct QueuedInput {
        Key key = Key::None;
        bool pressed = false;
        Vec2f mouse_pos = {0, 0};
    };
    std::queue<QueuedInput> input_queue_;

public:
    bool init(const BackendConfig& config) override {
        config_ = config;
        config_.headless = true;
        fb_width_ = config.window_width;
        fb_height_ = config.window_height;
        framebuffer_.resize(fb_width_ * fb_height_ * 4);  // RGBA
        return true;
    }
    
    void shutdown() override {
        framebuffer_.clear();
    }
    
    bool is_headless() const override { return true; }
    
    // For E2E testing: queue simulated inputs
    void queue_key_press(Key key) {
        input_queue_.push({key, true, {}});
    }
    
    void queue_mouse_move(Vec2f pos) {
        input_queue_.push({Key::None, false, pos});
    }
    
    // Clipboard (in-memory, perfect for testing)
    void set_clipboard_text(std::string_view text) override {
        clipboard_ = std::string(text);
    }
    
    std::string get_clipboard_text() override {
        return clipboard_;
    }
    
    bool has_clipboard_text() override {
        return !clipboard_.empty();
    }
    
    // Time (simulated, deterministic)
    double get_time() override {
        return simulated_time_;
    }
    
    float get_delta_time() override {
        return delta_time_;
    }
    
    void advance_time(float dt) {
        simulated_time_ += dt;
    }
    
    // Drawing captures to framebuffer for screenshot comparison
    void draw_rectangle(Rect rect, Color color) override {
        // Rasterize to framebuffer for testing
        int x0 = std::max(0, (int)rect.x);
        int y0 = std::max(0, (int)rect.y);
        int x1 = std::min(fb_width_, (int)(rect.x + rect.width));
        int y1 = std::min(fb_height_, (int)(rect.y + rect.height));
        
        for (int y = y0; y < y1; y++) {
            for (int x = x0; x < x1; x++) {
                int idx = (y * fb_width_ + x) * 4;
                framebuffer_[idx + 0] = color.r;
                framebuffer_[idx + 1] = color.g;
                framebuffer_[idx + 2] = color.b;
                framebuffer_[idx + 3] = color.a;
            }
        }
    }
    
    // Export framebuffer for test verification
    std::vector<uint8_t> get_framebuffer() const {
        return framebuffer_;
    }
    
    // ... other methods return safe defaults
};

}
```

---

## Backend Registry and Selection

```cpp
// afterhours/src/backends/backend_registry.h
namespace afterhours::backend {

enum class BackendType {
    Auto,       // Let library choose
    Raylib,
    Sokol,
    Headless,
    Custom
};

inline IBackend* current_backend = nullptr;

inline void set_backend(IBackend* backend) {
    current_backend = backend;
}

inline void set_backend(BackendType type) {
    switch (type) {
        case BackendType::Raylib:
            current_backend = new RaylibBackend();
            break;
        case BackendType::Sokol:
            current_backend = new SokolBackend();
            break;
        case BackendType::Headless:
            current_backend = new HeadlessBackend();
            break;
        case BackendType::Auto:
        default:
            #if defined(AFTER_HOURS_USE_SOKOL)
                current_backend = new SokolBackend();
            #elif defined(AFTER_HOURS_USE_RAYLIB)
                current_backend = new RaylibBackend();
            #else
                current_backend = new HeadlessBackend();
            #endif
            break;
    }
}

inline IBackend& get() {
    if (!current_backend) {
        set_backend(BackendType::Auto);
    }
    return *current_backend;
}

// Convenience functions that delegate to current backend
inline void set_clipboard_text(std::string_view text) {
    get().set_clipboard_text(text);
}

inline std::string get_clipboard_text() {
    return get().get_clipboard_text();
}

// ... etc for all other functions

}
```

---

## Sokol Integration Benefits

### 1. Cross-Platform Graphics API Support

Sokol automatically selects the best graphics API for each platform:

```cpp
// Compile-time backend selection
#if defined(_WIN32)
    #define SOKOL_D3D11      // Direct3D 11 on Windows
#elif defined(__APPLE__)
    #define SOKOL_METAL      // Metal on macOS/iOS
#elif defined(__EMSCRIPTEN__)
    #define SOKOL_WGPU       // WebGPU for modern browsers
    // or SOKOL_GLES3 for WebGL2
#else
    #define SOKOL_GLCORE33   // OpenGL 3.3 on Linux
#endif
```

### 2. WebAssembly First-Class Support

Sokol is designed with Emscripten in mind:

```cpp
// Async loading with sokol_fetch.h
void load_font_async(const char* path, FontLoadCallback cb) {
    sfetch_request(&(sfetch_request_t){
        .path = path,
        .callback = [](const sfetch_response_t* response) {
            if (response->finished && !response->failed) {
                // Font data in response->data.ptr
                cb(response->data.ptr, response->data.size);
            }
        }
    });
}
```

### 3. Memory Tracking for Debugging

```cpp
#define SOKOL_MEMTRACK_IMPL
#include "sokol_memtrack.h"

void report_memory_usage() {
    smemtrack_info_t info = smemtrack_info();
    printf("Allocations: %d, Total: %zu bytes\n", 
           info.num_allocs, info.num_bytes);
}
```

### 4. Built-in Debug Text Rendering

```cpp
#include "sokol_debugtext.h"

void draw_debug_overlay() {
    sdtx_canvas(sapp_width() * 0.5f, sapp_height() * 0.5f);
    sdtx_origin(1.0f, 1.0f);
    sdtx_color3b(255, 255, 0);
    sdtx_printf("FPS: %.1f\n", 1.0f / delta_time_);
    sdtx_printf("Frame: %zu\n", frame_count_);
    sdtx_draw();
}
```

---

## Migration Path

### Phase 1: Abstract Current Usage

1. Create `IBackend` interface
2. Wrap existing Raylib calls in `RaylibBackend`
3. Update Afterhours public API to delegate to backend

### Phase 2: Implement Sokol Backend

1. Add Sokol headers to vendor
2. Implement `SokolBackend` class
3. Add compile-time selection via `AFTER_HOURS_USE_SOKOL`

### Phase 3: Add Headless Backend

1. Implement `HeadlessBackend` for testing
2. Update E2E test infrastructure to use headless mode
3. Add framebuffer capture for visual regression tests

### Phase 4: Documentation and Examples

1. Document backend selection
2. Provide examples for each backend
3. Migration guide from direct Raylib usage

---

## Build Configuration

```makefile
# Use Sokol backend
CXXFLAGS += -DAFTER_HOURS_USE_SOKOL

# Platform-specific graphics API
ifeq ($(OS),Windows_NT)
    CXXFLAGS += -DSOKOL_D3D11
else ifeq ($(shell uname),Darwin)
    CXXFLAGS += -DSOKOL_METAL
    LDFLAGS += -framework Metal -framework MetalKit
else
    CXXFLAGS += -DSOKOL_GLCORE33
    LDFLAGS += -lGL
endif

# Or use Raylib backend
# CXXFLAGS += -DAFTER_HOURS_USE_RAYLIB

# Or headless for CI
# CXXFLAGS += -DAFTER_HOURS_USE_HEADLESS
```

---

## Affected Areas

This pattern should apply to all system integrations:

| Feature | Current State | Raylib Backend | Sokol Backend | Headless Backend |
|---------|--------------|----------------|---------------|------------------|
| Clipboard | Hardcoded raylib | ✅ SetClipboardText | ✅ sapp_set_clipboard_string | ✅ In-memory |
| File dialogs | Not in Afterhours | ❌ N/A | ❌ N/A (use NFD) | ✅ Mock paths |
| System fonts | Hardcoded raylib | ✅ LoadFont | ✅ sokol_fontstash | ✅ Stub |
| Mouse cursor | Hardcoded raylib | ✅ SetMouseCursor | ✅ sapp_set_mouse_cursor | ✅ Tracked state |
| Window management | Hardcoded raylib | ✅ InitWindow | ✅ sokol_app | ✅ Virtual window |
| Graphics rendering | Hardcoded raylib | ✅ rlgl | ✅ sokol_gfx + sokol_gl | ✅ Framebuffer |
| Audio | Hardcoded raylib | ✅ raudio | ✅ sokol_audio | ✅ Silent buffer |
| Time | Hardcoded raylib | ✅ GetTime | ✅ sokol_time | ✅ Simulated |

## Priority

**Medium** - The workaround is functional but requires discipline to use the app wrapper everywhere. Easy to accidentally call `afterhours::clipboard::` directly and break tests.

## Workaround Location

- `src/util/clipboard.h` - Application-layer clipboard wrapper with test mode support

## References

- [Sokol GitHub Repository](https://github.com/floooh/sokol)
- [Sokol Samples](https://github.com/floooh/sokol-samples)
- [Sokol Documentation](https://github.com/floooh/sokol/tree/master/doc)
- [Raylib vs Sokol comparison](https://github.com/AubakirovN/sokol-samples-stubs)
