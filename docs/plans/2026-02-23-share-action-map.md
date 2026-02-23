# Share ActionMap Instance Across Systems

**Date:** 2026-02-23
**Type:** Refactoring — reduce redundancy, enable runtime key remapping
**Impact:** Eliminates 2 redundant ActionMap copies, enables single-point key remapping

## Problem

Three systems each create their own `input::ActionMap` initialized with `createDefaultActionMap()`:

```cpp
// src/ecs/input_system.h
struct TextInputSystem : public afterhours::System<...> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();
    // ...
};

struct KeyboardShortcutSystem : public afterhours::System<...> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();
    // ...
};

struct NavigationSystem : public afterhours::System<...> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();
    // ...
};
```

`createDefaultActionMap()` populates an `unordered_map` with 80+ key bindings. This means:

1. **3 copies** of identical data in memory
2. **Key remapping is broken:** If you wanted to let users remap keys at runtime, you'd need to update all 3 maps. Currently there's no way to do this.
3. **Inconsistency risk:** If someone adds a binding to one map but forgets the others.

## Proposed Solution

### Option A: Use the existing `InputComponent` (recommended)

`InputComponent` already exists in `components.h` and holds an `ActionMap`:

```cpp
struct InputComponent : public afterhours::BaseComponent {
    input::ActionMap actionMap;
    InputComponent() : actionMap(input::createDefaultActionMap()) {}
};
```

But it's never queried by any system. The entity has it added (via `addComponent<InputComponent>()` — actually, checking the code, it's NOT added to the entity). 

Solution: Add `InputComponent` to the editor entity in `app_init()`, and have all three systems query for it:

```cpp
struct TextInputSystem : public afterhours::System<DocumentComponent, CaretComponent, MenuComponent, InputComponent> {
    void for_each_with(Entity&, DocumentComponent& doc, CaretComponent& caret,
                       MenuComponent& menu, InputComponent& input, float) override {
        // Use input.actionMap instead of actionMap_
    }
};
```

### Option B: Shared pointer / singleton

Make `ActionMap` a singleton or pass a shared pointer. Less ECS-idiomatic but simpler.

### Option C: Static ActionMap

Since the map is the same for all systems and never changes at runtime (currently), make it a function-local static:

```cpp
static const input::ActionMap& getDefaultActionMap() {
    static input::ActionMap map = input::createDefaultActionMap();
    return map;
}
```

This is the simplest change but doesn't support runtime remapping.

## Files to Change

| File | Change |
|------|--------|
| `src/main.cpp` | Add `InputComponent` to editor entity (if using Option A) |
| `src/ecs/input_system.h` | Remove member `actionMap_` from all 3 systems, query `InputComponent` instead |
| `src/ecs/components.h` | Verify `InputComponent` definition (already exists) |

## Risks

- Option A changes system template parameters, which affects ECS query behavior
- If using Option A, all 3 systems must find the same entity — they already do (all components are on the editor entity)
- `ActionMap::isActionPressed` and `isActionPressedRepeat` iterate all bindings linearly — having one map instead of 3 doesn't change per-check cost but avoids 3x the memory

## Migration Strategy

1. Add `InputComponent` to editor entity in `app_init()` (if not already present)
2. Update `NavigationSystem` first (simplest system to test — arrow keys)
3. Verify navigation E2E tests pass
4. Update `TextInputSystem` and `KeyboardShortcutSystem`
5. Remove the 3 member `actionMap_` fields
