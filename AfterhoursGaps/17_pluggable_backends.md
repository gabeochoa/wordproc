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

## Proposed Solution

Afterhours should support pluggable backends through a registration pattern:

```cpp
namespace afterhours::clipboard {

// Backend interface
struct Backend {
    virtual ~Backend() = default;
    virtual void set_text(std::string_view text) = 0;
    virtual std::string get_text() = 0;
    virtual bool has_text() = 0;
};

// Default implementations
struct RaylibBackend : Backend {
    void set_text(std::string_view text) override {
        raylib::SetClipboardText(std::string(text).c_str());
    }
    std::string get_text() override {
        const char* t = raylib::GetClipboardText();
        return t ? t : "";
    }
    bool has_text() override {
        const char* t = raylib::GetClipboardText();
        return t && t[0] != '\0';
    }
};

struct InMemoryBackend : Backend {
    std::string buffer;
    void set_text(std::string_view text) override { buffer = text; }
    std::string get_text() override { return buffer; }
    bool has_text() override { return !buffer.empty(); }
};

// Global backend management
namespace detail {
    inline std::unique_ptr<Backend> current_backend;
    inline Backend& get_backend() {
        if (!current_backend) {
#ifdef AFTER_HOURS_USE_RAYLIB
            current_backend = std::make_unique<RaylibBackend>();
#else
            current_backend = std::make_unique<InMemoryBackend>();
#endif
        }
        return *current_backend;
    }
}

inline void set_backend(std::unique_ptr<Backend> backend) {
    detail::current_backend = std::move(backend);
}

inline void use_in_memory_backend() {
    set_backend(std::make_unique<InMemoryBackend>());
}

// Public API delegates to current backend
inline void set_text(std::string_view text) {
    detail::get_backend().set_text(text);
}

inline std::string get_text() {
    return detail::get_backend().get_text();
}

inline bool has_text() {
    return detail::get_backend().has_text();
}

}
```

## Usage After Fix

```cpp
// In test setup
afterhours::clipboard::use_in_memory_backend();

// Or custom backend
afterhours::clipboard::set_backend(std::make_unique<MyLoggingBackend>());

// API usage unchanged
afterhours::clipboard::set_text("test");
```

## Affected Areas

This pattern should apply to all system integrations:

| Feature | Current State | Needs Backend |
|---------|--------------|---------------|
| Clipboard | Hardcoded raylib | ✅ Yes |
| File dialogs | Not in Afterhours | Would benefit |
| System fonts | Hardcoded raylib | ✅ Yes |
| Mouse cursor | Hardcoded raylib | ✅ Yes |
| Window management | Hardcoded raylib | ✅ Yes |

## Priority

**Medium** - The workaround is functional but requires discipline to use the app wrapper everywhere. Easy to accidentally call `afterhours::clipboard::` directly and break tests.

## Workaround Location

- `src/util/clipboard.h` - Application-layer clipboard wrapper with test mode support

