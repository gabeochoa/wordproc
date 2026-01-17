# Uninitialized Element Size Warning

## Problem
Autolayout skips computing sizes for elements created after `RunAutoLayout`
executes. These elements render with `computed[X/Y] == -1`, which can surface as
"Container too small for text" warnings or undefined layout behavior.

## Root Cause
System ordering matters: UI-creating systems must run *before* `RunAutoLayout`.
If a system spawns UI elements after layout, the new elements never receive a
computed size in that frame.

## Current Workaround
Split UI registration into pre/post layout phases:
- `registerUIPreLayoutSystems()` for UI creation
- `registerUIPostLayoutSystems()` for dependent UI logic

Custom UI systems (like `MenuUISystem`) are registered between those phases.

## Suggested Afterhours Change
Add a warning/assert in rendering when size is uninitialized:

```cpp
if (widget.computed[Axis::X] == -1 || widget.computed[Axis::Y] == -1) {
  log_warn("Rendering element '{}' with uninitialized size (-1). "
           "Ensure UI-creating systems run before RunAutoLayout.",
           widget.debug_name);
}
```

## Notes
The warning helps catch ordering bugs early without changing layout behavior.

