# Render System Const Constraint

## Problem
Afterhours render systems only call the `const` version of `for_each_with`, which receives
`const Entity&` and `const Component&...` references. This makes it impossible to implement
immediate-mode UI patterns where the render pass needs to update component state (e.g., closing
a dialog when a button is clicked, updating scroll offsets, toggling visibility flags).

## Evidence
In `SystemManager::render()`:
```cpp
void render(const Entities &entities, const float dt) {
    for (const auto &system : render_systems_) {
        const SystemBase &sys = *system;  // CONST reference
        sys.once(dt);
        for (std::shared_ptr<Entity> entity : entities) {
            const Entity &e = *entity;     // CONST reference
            sys.for_each(e, dt);           // Calls CONST for_each
        }
    }
}
```

This calls `for_each(const Entity&, float) const`, which only invokes the const version of
`for_each_with`. Systems that only implement the mutable version are silently ignored.

## Current Workaround
- Use `const_cast` to obtain mutable references in the const `for_each_with`:
```cpp
void for_each_with(const Entity& entity,
                   const DocumentComponent& docConst,
                   const MenuComponent& menuConst,
                   const float) const override {
    auto& doc = const_cast<DocumentComponent&>(docConst);
    auto& menu = const_cast<MenuComponent&>(menuConst);
    // ... immediate-mode UI that modifies state
}
```
- Alternatively, move all UI interaction logic to update systems and use render systems
  only for pure drawing (separating interaction and rendering).

## Desired Behavior
Either:
1. **Allow mutable render systems**: Provide an opt-in mechanism for render systems that
   need mutable access (e.g., `register_mutable_render_system()`).
2. **Immediate-mode UI support**: Provide a blessed way to capture UI interactions during
   rendering and defer state changes to a post-render callback.
3. **Document the constraint**: If const-only is intentional, document that render systems
   cannot modify component state and suggest the update/render split pattern.

## Proposed API Sketch
Option 1 - Mutable render systems:
```cpp
// New registration function
void register_mutable_render_system(std::unique_ptr<SystemBase> system);

// In SystemManager::render(), call non-const for_each for mutable systems
```

Option 2 - Deferred state changes:
```cpp
// Render systems can queue state changes
ui::defer([&menu]() { menu.showDialog = false; });

// Changes are applied after rendering completes
```

## Notes
Immediate-mode UI (like Win95/ImGui-style widgets) inherently mixes rendering and state
updates. Requiring const-only render systems forces awkward patterns like `const_cast`
or complex update/render system splits for simple UI interactions.

