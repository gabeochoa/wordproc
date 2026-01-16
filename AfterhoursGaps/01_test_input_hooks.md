# Test Input Hooks

## Working Implementation
See these files for a complete working example:
- `src/testing/test_input.h` - Main API for test input injection
- `src/testing/test_input.cpp` - Implementation
- `src/testing/input_injector.h` - Low-level input injection
- `src/testing/input_injector.cpp` - Implementation
- `src/testing/test_input_provider.h` - UIContext integration

## Problem
Afterhours lacks a first-class way to inject input events for automated tests.
The library provides no concrete API for simulated key/mouse input, requiring
applications to build their own testing infrastructure with raylib overrides.

## Current Workaround

### Architecture
The app maintains a two-layer test input system:

1. **`src/testing/test_input.h/.cpp`** - High-level test input API
   - Manages simulated mouse state (`MouseState` struct)
   - Handles key/char queue for keyboard simulation
   - Integrates with Afterhours `UIContext` via `TestInputProvider`

2. **`src/testing/input_injector.h/.cpp`** - Low-level injection
   - Directly manipulates raylib state where possible
   - Handles click scheduling and key hold simulation

3. **`src/external.h`** - Macro redefinition layer
   - Redefines raylib input functions to use test wrappers:
     ```cpp
     #define IsMouseButtonPressed IsMouseButtonPressed_Test
     #define IsMouseButtonDown IsMouseButtonDown_Test
     #define GetMousePosition GetMousePosition_Test
     // etc.
     ```

### Key Implementation Details

#### Mouse Click Timing Issue
A significant challenge is that `IsMouseButtonPressed()` should return `true` for
exactly ONE frame (the frame the button transitions from up to down). The E2E
script runner executes AFTER systems run, so a naive implementation fails:

```
Frame N:   E2E sets pressed=true → systems already ran → not seen
Frame N+1: reset_frame clears flag → systems run → pressed=false!
```

**Solution:** Use a `frames_until_clear_press` counter:
```cpp
struct MouseState {
    bool left_button_pressed_this_frame = false;
    int frames_until_clear_press = 0;  // Counter for timing
    // ...
};

void simulate_mouse_button_press(int button) {
    mouse_state.left_button_pressed_this_frame = true;
    mouse_state.frames_until_clear_press = 1;  // Keep active for 1 frame
}

void reset_frame() {
    if (mouse_state.frames_until_clear_press > 0) {
        mouse_state.frames_until_clear_press--;
        // Don't clear pressed state yet
    } else {
        mouse_state.left_button_pressed_this_frame = false;
    }
}
```

#### Mouse Position Simulation
The simulated mouse position must be returned by `GetMousePosition()` for
hover detection to work in UI code:
```cpp
vec2 get_mouse_position() {
    if (test_mode && mouse_state.simulation_active &&
        mouse_state.position.has_value()) {
        return mouse_state.position.value();
    }
    return input_injector::get_mouse_position();
}
```

#### UIContext Integration
For Afterhours components (buttons, etc.) to see simulated input, we need
`TestInputProvider` to update `UIContext.mouse`:
```cpp
void for_each_with(Entity&, UIContext<InputAction>& context, float) override {
    if (provider->mouse_position.has_value()) {
        context.mouse.pos = provider->mouse_position.value();
    }
    context.mouse.left_down = provider->mouse_left_down;
}
```

### Files Involved
- `src/testing/test_input.h` - MouseState, key queue declarations
- `src/testing/test_input.cpp` - Implementation of simulation logic
- `src/testing/test_input_fwd.h` - Forward declarations for external.h
- `src/testing/test_input_provider.h` - Afterhours system integration
- `src/testing/input_injector.h/.cpp` - Low-level raylib injection
- `src/external.h` - Macro wrappers for raylib functions

## Desired Behavior

Afterhours should provide a built-in test input subsystem that:

1. **Frame-accurate input injection** - Simulated presses visible for exactly
   one frame, matching real hardware behavior

2. **Transparent integration** - No macro hacks; UIContext should accept
   an optional input provider interface

3. **Deterministic delivery** - Input consumed in predictable order across
   frames, enabling reproducible tests

4. **No global state** - Per-context test input, allowing parallel tests

## Proposed API Sketch

```cpp
namespace afterhours::ui::test {

// Input simulation
void push_key(int keycode);
void push_char(char32_t c);
void set_mouse_position(float x, float y);
void press_mouse_button(int button);  // Triggers IsMouseButtonPressed for 1 frame
void hold_mouse_button(int button);   // Makes IsMouseButtonDown return true
void release_mouse_button(int button);
void scroll_wheel(float delta);

// Frame control
void advance_frame();  // Clears per-frame state, advances queues
void clear_all();      // Reset all simulated state

// Configuration
void enable_test_mode(UIContext& ctx);
void disable_test_mode(UIContext& ctx);

}  // namespace afterhours::ui::test
```

## Alternative: Provider Interface

Instead of global functions, use an interface that UIContext consumes:

```cpp
struct InputProvider {
    virtual ~InputProvider() = default;
    virtual Vector2 get_mouse_position() = 0;
    virtual bool is_mouse_button_pressed(int button) = 0;
    virtual bool is_mouse_button_down(int button) = 0;
    virtual bool is_key_pressed(int key) = 0;
    virtual int get_char_pressed() = 0;
};

// Default: delegates to raylib
struct RaylibInputProvider : InputProvider { ... };

// Test: returns simulated values
struct TestInputProvider : InputProvider { ... };

template <typename InputAction>
struct UIContext {
    InputProvider* input = &default_raylib_provider;
    // ...
};
```

This allows swapping input sources without macro hacks and enables
per-context configuration.

## Notes

- The current workaround requires careful coordination between E2E script
  execution timing and system execution order
- Immediate-mode UI (win95 widgets) using raw raylib calls need the macro
  wrappers; Afterhours UI could use the provider interface
- Test screenshots should be taken AFTER rendering completes to capture
  the visual state corresponding to the test assertions
