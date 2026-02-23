# Deduplicate Entity Queries Across UI Systems

**Date:** 2026-02-23
**Type:** Refactoring — cleaner ECS patterns, minor performance improvement
**Impact:** ~80 lines of boilerplate removed, ~15 redundant hash-map lookups per frame eliminated

## Problem

Every UI system manually queries for the same components using `EntityQuery`:

```cpp
auto docEntities = afterhours::EntityQuery({.force_merge = true})
    .whereHasComponent<DocumentComponent>().gen();
if (docEntities.empty()) return;
auto& doc = docEntities[0].get().get<DocumentComponent>();
```

This 4-line pattern appears:
- `ToolbarRenderSystem`: 3 queries (Document, Layout, Toolbar)
- `MenuUISystem`: 5+ queries (Menu, Layout, Document — some inside dialog branches)
- `StatusBarSystem`: 2 queries (Layout, Document)
- `TitleBarSystem`: 2 queries (Layout, Document)
- `MenuSystem`: 0 (uses template params correctly)
- `EditorRenderSystem`: 0 (uses template params correctly)

Total: ~15 redundant queries per frame, all finding the same single entity.

## Why This Happens

`MenuUISystem`, `ToolbarRenderSystem`, `StatusBarSystem`, and `TitleBarSystem` all declare their system template as `System<UIContext<InputAction>>` — they only query for the UI context singleton. They then manually query for `DocumentComponent`, `LayoutComponent`, etc. inside `for_each_with`.

Meanwhile, `EditorRenderSystem` correctly uses `System<DocumentComponent, CaretComponent, ScrollComponent, LayoutComponent, MenuComponent>` and gets all components passed as parameters.

## Proposed Solution

Change each system's template parameters to include the components it needs:

```cpp
// Before
struct StatusBarSystem : afterhours::System<UIContext<InputAction>> {
    void for_each_with(Entity&, UIContext<InputAction>& ctx, float) override {
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
            .whereHasComponent<LayoutComponent>().gen();
        if (layoutEntities.empty()) return;
        auto& layout = layoutEntities[0].get().get<LayoutComponent>();
        // ... same for DocumentComponent ...
    }
};

// After
struct StatusBarSystem : afterhours::System<UIContext<InputAction>, LayoutComponent, DocumentComponent> {
    void for_each_with(Entity&, UIContext<InputAction>& ctx,
                       LayoutComponent& layout, DocumentComponent& doc, float) override {
        // Components are already available — no queries needed
    }
};
```

### Caveat

This approach works if `UIContext<InputAction>` and the other components are on the same entity, OR if `force_merge` is the default behavior for multi-component system queries in afterhours. If `UIContext` lives on a different entity than `DocumentComponent`, this won't work directly.

**Alternative:** If entities are separate, keep `System<UIContext<InputAction>>` but cache entity references at init time (the editor entity never changes). Use a base class or helper:

```cpp
struct UISystemBase {
    ecs::DocumentComponent* doc_ = nullptr;
    ecs::LayoutComponent* layout_ = nullptr;
    void resolveEntities() {
        if (doc_) return; // already resolved
        auto entities = afterhours::EntityQuery({.force_merge = true})
            .whereHasComponent<ecs::DocumentComponent>().gen();
        if (!entities.empty()) {
            auto& e = entities[0].get();
            doc_ = &e.get<ecs::DocumentComponent>();
            layout_ = &e.get<ecs::LayoutComponent>();
        }
    }
};
```

## Files to Change

| File | Change |
|------|--------|
| `src/ecs/toolbar_system.h` | Change system template or cache entities |
| `src/ecs/menu_ui_system.h` | Change system template or cache entities |
| `src/ecs/status_bar_system.h` | Change system template or cache entities |
| `src/ecs/title_bar_system.h` | Change system template or cache entities |

## Risks

- Need to verify how afterhours handles `System<A, B>` when A and B are on different entities (force_merge behavior)
- If using cached references, must handle the case where the entity is recreated (unlikely in this app)

## Migration Strategy

1. Test with one system first (`StatusBarSystem` — simplest, only 2 queries)
2. Verify E2E tests pass
3. Apply same pattern to remaining systems
