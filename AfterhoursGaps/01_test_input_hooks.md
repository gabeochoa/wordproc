# Test Input Hooks

## Status: ADDRESSED ✅

Afterhours now provides test input hooks in `vendor/afterhours/src/plugins/e2e_testing/`:
- `input_injector.h` - Low-level synthetic key/mouse state
- `test_input.h` - High-level input queue with backend wrapping

### Wordproc Integration (Completed)
Wordproc now uses afterhours directly via `src/external.h`:
- Namespace aliases: `test_input::` → `afterhours::testing::test_input::`
- Namespace aliases: `input_injector::` → `afterhours::testing::input_injector::`
- Macro wrappers intercept raylib input calls and route through test input when enabled

**Deleted files** (no longer needed):
- `src/testing/test_input.h` - functionality moved to external.h
- `src/testing/test_input.cpp`
- `src/testing/test_input_fwd.h`
- `src/testing/test_input_provider.h`

## Original Problem (now solved)
Afterhours previously lacked a first-class way to inject input events for automated tests.
The library now provides a concrete API for simulated key/mouse input.

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

## Missing: Mouse Wheel Input

The current API proposal is missing mouse wheel simulation, which is needed for 
testing scrollable containers (see **08_scrollable_containers.md**).

**Add to the API:**

```cpp
namespace afterhours::ui::test {

// Existing API...
void push_key(int keycode);
void push_char(char32_t c);
void set_mouse_position(float x, float y);
void press_mouse_button(int button);
void hold_mouse_button(int button);
void release_mouse_button(int button);

// NEW: Mouse wheel simulation
void scroll_wheel(float delta);            // Vertical wheel
void scroll_wheel_v(float dx, float dy);   // 2D trackpad scrolling

}
```

**Internal state addition:**

```cpp
struct MouseState {
  // ...existing members...
  float wheel_delta = 0.f;
  Vector2Type wheel_delta_v = {0.f, 0.f};
  int frames_until_clear_wheel = 0;  // Same timing pattern as clicks
};
```

**InputProvider interface extension:**

```cpp
struct InputProvider {
  // ...existing...
  virtual float get_mouse_wheel_move() = 0;
  virtual Vector2 get_mouse_wheel_move_v() = 0;
};
```

## Related Gaps

- **08_scrollable_containers.md** - Scroll testing requires wheel input injection
- **12_e2e_testing_framework.md** - E2E DSL needs `scroll` command using this API

## Notes

- The current workaround requires careful coordination between E2E script
  execution timing and system execution order
- Immediate-mode UI (win95 widgets) using raw raylib calls need the macro
  wrappers; Afterhours UI could use the provider interface
- Test screenshots should be taken AFTER rendering completes to capture
  the visual state corresponding to the test assertions
