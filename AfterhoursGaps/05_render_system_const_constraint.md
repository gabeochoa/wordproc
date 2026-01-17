# Render System Const Constraint

## Problem
Afterhours render systems only call the `const` version of `for_each_with`, which receives
`const Entity&` and `const Component&...` references. This makes it impossible to implement
immediate-mode UI patterns where the render pass needs to update component state (e.g., closing
a dialog when a button is clicked, updating scroll offsets, toggling visibility flags).

## Root Cause Analysis

### Code Path
In `vendor/afterhours/src/core/system.h`, `SystemManager::render()` (lines 393-410):

```cpp
void render(const Entities &entities, const float dt) {
    for (const auto &system : render_systems_) {
        if (!system->should_run(dt))
            continue;
        const SystemBase &sys = *system;     // <-- CONST cast here
        sys.once(dt);
        for (std::shared_ptr<Entity> entity : entities) {
            if (!entity)
                continue;
            const Entity &e = *entity;       // <-- CONST cast here
            if (sys.include_derived_children)
                sys.for_each_derived(e, dt); // Calls const version
            else
                sys.for_each(e, dt);         // Calls const version
        }
        sys.after(dt);
    }
}
```

Compare with `tick()` (lines 358-374), which does NOT const-cast:
```cpp
void tick(Entities &entities, const float dt) {
    for (auto &system : update_systems_) {
        // ... 
        system->for_each(*entity, dt);       // <-- Non-const, calls mutable for_each_with
    }
}
```

### Template Dispatch Mechanism
The `System` template has two dispatch paths (lines 120-133):
- `CallWithComponents::call()` → calls non-const `for_each_with(Entity&, Components&...)`
- `CallWithComponents::call_const()` → calls const `for_each_with(const Entity&, const Components&...)`

When `render()` uses `const SystemBase &sys`, only `call_const` is invoked, so systems
implementing only the mutable `for_each_with` are **silently ignored** (the empty virtual
base implementation runs instead).

## Current Workarounds in Use

### Workaround 1: `const_cast` in Render Systems (Used in wordproc)
From `src/ecs/render_system.h` (lines 648-651, 808-811):
```cpp
void for_each_with(const Entity& /*entity*/,
                   const DocumentComponent& docConst,
                   const MenuComponent& menuConst,
                   const float) const override {
    // const_cast for immediate-mode UI that needs to update state during draw
    auto& doc = const_cast<DocumentComponent&>(docConst);
    auto& menu = const_cast<MenuComponent&>(menuConst);
    renderMenus(doc, menu, status, layout);
}
```

**Pros:** Works, no changes to Afterhours needed
**Cons:** Technically undefined behavior, easy to forget, verbose boilerplate

### Workaround 2: `mutable` Member Variables (Used in Afterhours itself)
From `vendor/afterhours/src/plugins/ui/rendering.h` (lines 360-367):
```cpp
struct RenderImm : System<UIContext<InputAction>, FontManager> {
    mutable UIContext<InputAction> *context;
    mutable int level = 0;
    mutable int indent = 0;
    mutable EntityID isolated_id = -1;
    mutable bool isolate_enabled = false;
    // ...
};
```

Also in `texture_manager.h` (lines 205, 221):
```cpp
struct RenderSprites : System<HasSprite> {
    mutable Texture sheet;
    // ...
};
```

**Pros:** Allows system-owned state mutation, well-defined behavior
**Cons:** Only works for system members, not for ECS components passed to `for_each_with`

### Workaround 3: Update/Render Split
Move all interaction logic to update systems, use render systems purely for drawing.

**Pros:** Clean separation, matches ECS philosophy
**Cons:** Breaks immediate-mode UI paradigm, complex for simple interactions

## Proposed Solutions (for Afterhours PR)

### Option A: Add `register_mutable_render_system()` (Recommended)

**Implementation:** Add a flag to track which render systems need mutable access, and call
the non-const `for_each` for those systems.

**Changes to `system.h`:**

```cpp
struct SystemManager {
    // ...existing members...
    std::vector<std::unique_ptr<SystemBase>> render_systems_;
    std::vector<bool> render_system_is_mutable_;  // NEW: parallel vector for mutability flag

    // NEW: Registration function for mutable render systems
    void register_mutable_render_system(std::unique_ptr<SystemBase> system) {
        render_systems_.emplace_back(std::move(system));
        render_system_is_mutable_.push_back(true);
    }

    void register_render_system(std::unique_ptr<SystemBase> system) {
        render_systems_.emplace_back(std::move(system));
        render_system_is_mutable_.push_back(false);
    }

    // MODIFIED render() function
    void render(Entities &entities, const float dt) {  // Note: non-const Entities now
        for (size_t i = 0; i < render_systems_.size(); ++i) {
            auto &system = render_systems_[i];
            if (!system->should_run(dt))
                continue;
            
            if (render_system_is_mutable_[i]) {
                // Mutable path - same as tick()
                system->once(dt);
                for (std::shared_ptr<Entity> entity : entities) {
                    if (!entity) continue;
                    if (system->include_derived_children)
                        system->for_each_derived(*entity, dt);
                    else
                        system->for_each(*entity, dt);
                }
                system->after(dt);
            } else {
                // Const path - existing behavior
                const SystemBase &sys = *system;
                sys.once(dt);
                for (std::shared_ptr<Entity> entity : entities) {
                    if (!entity) continue;
                    const Entity &e = *entity;
                    if (sys.include_derived_children)
                        sys.for_each_derived(e, dt);
                    else
                        sys.for_each(e, dt);
                }
                sys.after(dt);
            }
        }
    }

    void render_all(const float dt) {
        auto &entities = EntityHelper::get_entities_for_mod();  // Changed from get_entities()
        render(entities, dt);
    }
};
```

**Migration for wordproc:**
```cpp
// Before
sm.register_render_system(std::make_unique<MenuSystem>());

// After - in systems that need mutable access
sm.register_mutable_render_system(std::make_unique<MenuSystem>());

// And update MenuSystem to use non-const for_each_with:
void for_each_with(Entity& entity,
                   DocumentComponent& doc,
                   MenuComponent& menu,
                   float dt) override {
    renderMenus(doc, menu, status, layout);
}
```

**Pros:**
- Backwards compatible (existing const render systems unchanged)
- Opt-in semantics make intent clear
- Removes need for `const_cast`
- Minimal code changes

**Cons:**
- Parallel vector could get out of sync (alternative: use `std::pair` or struct)

### Option B: Use `wants_mutable_access` Virtual Method

**Implementation:** Add a virtual method to `SystemBase` that systems can override.

```cpp
class SystemBase {
public:
    // ...existing...
    virtual bool wants_mutable_access() const { return false; }
};

// In SystemManager::render()
void render(Entities &entities, const float dt) {
    for (auto &system : render_systems_) {
        if (!system->should_run(dt)) continue;
        
        if (system->wants_mutable_access()) {
            // mutable path
            system->once(dt);
            for (auto entity : entities) {
                if (!entity) continue;
                system->for_each(*entity, dt);
            }
            system->after(dt);
        } else {
            // const path (existing)
            // ...
        }
    }
}
```

Then in user systems:
```cpp
struct MenuSystem : System<DocumentComponent, MenuComponent> {
    bool wants_mutable_access() const override { return true; }
    
    void for_each_with(Entity&, DocumentComponent& doc, MenuComponent& menu, float) override {
        // Now gets called with mutable references
    }
};
```

**Pros:**
- Self-documenting in the system class itself
- No registration API change
- Can't get out of sync

**Cons:**
- Virtual call overhead (negligible)
- Systems must remember to override

### Option C: Deferred Command Queue

**Implementation:** Provide a mechanism to queue state changes during render that execute after.

```cpp
namespace afterhours::ui {
    inline std::vector<std::function<void()>> deferred_commands;
    
    inline void defer(std::function<void()> cmd) {
        deferred_commands.push_back(std::move(cmd));
    }
    
    inline void flush_deferred() {
        for (auto& cmd : deferred_commands) cmd();
        deferred_commands.clear();
    }
}

// In SystemManager::render(), after all systems run:
void render(...) {
    for (const auto &system : render_systems_) {
        // ... existing const dispatch ...
    }
    ui::flush_deferred();  // Apply queued changes
}

// Usage in render system:
void for_each_with(const Entity&, const MenuComponent& menu, float) const override {
    if (button_clicked) {
        ui::defer([&menu]() {
            const_cast<MenuComponent&>(menu).showDialog = false;
        });
    }
}
```

**Pros:**
- Works with existing const systems
- Changes applied atomically after render
- Useful for other deferred patterns

**Cons:**
- Still requires reference capture
- Complexity for simple operations
- Lifetime issues with captured references

## Recommendation

**Implement Option A (register_mutable_render_system)** as the primary solution:
1. Minimal API surface change
2. Backwards compatible
3. Clear opt-in semantics
4. Aligns with how `tick()` already works

Consider Option B as an alternative if the Afterhours maintainers prefer not changing
registration functions.

## Implementation Checklist

- [ ] Modify `SystemManager` in `vendor/afterhours/src/core/system.h`
  - [ ] Add `render_system_is_mutable_` tracking (or use Option B's virtual method)
  - [ ] Add `register_mutable_render_system()` function
  - [ ] Update `render()` to dispatch based on mutability
  - [ ] Update `render_all()` to use `get_entities_for_mod()`
- [ ] Add unit test demonstrating mutable render system works
- [ ] Update `vendor/afterhours/README.md` with new registration option
- [ ] Migrate wordproc:
  - [ ] Change `MenuSystem` to use mutable `for_each_with`
  - [ ] Change `RenderDocumentSystem` to use mutable `for_each_with`
  - [ ] Remove all `const_cast` workarounds
  - [ ] Register with `register_mutable_render_system()`

## Dependent Features

This constraint blocks several other Afterhours gaps:

### 08_scrollable_containers.md - BLOCKED

The scroll container system requires mutable render access for:

1. **Scroll position updates** - Mouse wheel input during render ideally updates 
   `HasScrollView::scroll_offset` immediately

2. **Scrollbar interaction** - Dragging scrollbar thumb updates scroll position

3. **Hover state feedback** - Scrollbar visual state changes on hover

Without this fix, scroll containers must either:
- Split into update system (input) + render system (drawing) - loses immediate-mode feel
- Use `const_cast` everywhere - undefined behavior, error-prone
- Use `mutable` on all scroll state - awkward component design

### 09_modal_dialogs.md - BLOCKED

Modal dialogs need to update state during render:
- Close dialog when backdrop clicked
- Update button hover/active states
- Handle Escape key for dismissal

### 03_text_editing_widget.md - PARTIALLY BLOCKED

Text editor cursor blinking, selection updates, and scroll-to-cursor all want 
immediate-mode patterns that conflict with const render.

## Notes
- Afterhours TODO already mentions this: "Const system registration issue" (system.h:159-161)
- The library itself uses `mutable` members extensively (see `RenderImm`, `RenderSprites`)
- Immediate-mode UI (ImGui-style) inherently mixes rendering and state updates
- Entity merging (`EntityHelper::merge_entity_arrays()`) only happens in `tick()`, so
  render systems creating entities would need careful handling
