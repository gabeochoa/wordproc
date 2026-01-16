# Test Input Hooks

## Problem
Afterhours lacks a first-class way to inject input events for automated tests.
`src/external.h` expects test input helpers, but the library provides no concrete
API for simulated key or mouse input.

## Current Workaround
- App maintains its own test input queue and mouse state in `src/testing/`.
- Raylib input calls are wrapped or re-exported to support synthetic events.

## Desired Behavior
- Provide a test input subsystem that can be enabled per test run.
- Allow enqueueing key presses (including character input) and mouse events.
- Offer deterministic, frame-driven delivery of queued input events.
- Expose APIs that do not require macro-undef hacks to intercept input.

## Proposed API Sketch
- `ui::test::push_key(keycode)`
- `ui::test::push_char(char32_t)`
- `ui::test::set_mouse_position(x, y)`
- `ui::test::click_mouse(button)`
- `ui::test::clear()`

## Notes
This would let tests drive the UI without relying on raylib overrides or
global state.
# Test Input Hooks

## Problem
Afterhours lacks a first-class way to inject input events for automated tests.
`src/external.h` expects test input helpers, but the library provides no concrete
API for simulated key/mouse input.

## Current Workaround
- App maintains its own test input queue and mouse state in `src/testing/`.
- Raylib input calls are wrapped or re-exported to support synthetic events.

## Desired Behavior
- Provide a test input subsystem that can be enabled per test run.
- Allow enqueueing key presses (including character input) and mouse events.
- Offer deterministic, frame-driven delivery of queued input events.
- Expose APIs that do not require macro-undef hacks to intercept input.

## Proposed API Sketch
- `ui::test::push_key(keycode)`
- `ui::test::push_char(char32_t)`
- `ui::test::set_mouse_position(x, y)`
- `ui::test::click_mouse(button)`
- `ui::test::clear()`

## Notes
This would let tests drive the UI without relying on raylib overrides or
global state.

