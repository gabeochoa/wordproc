# Extract a Unified Document Command Layer

**Date:** 2026-02-23
**Type:** Refactoring — reduce duplication, single source of truth
**Impact:** ~400 lines removed, eliminates bug class where fixes apply to one code path but not others

## Problem

The same document operations (Save, Open, New, Bold, Undo, etc.) are implemented independently in 3+ places:

- `handleMenuActionImpl` — 900-line switch statement in `src/ecs/render_system.h`
- `KeyboardShortcutSystem` — in `src/ecs/input_system.h`
- `ToolbarRenderSystem` — in `src/ecs/toolbar_system.h`
- `app_frame()` pending dialog handlers — in `src/main.cpp`

### Concrete examples of duplication

**Save** is copy-pasted with the same layout-sync + `saveDocumentEx` + toast sequence in:
1. `handleMenuActionImpl` case 3 (lines 943–980 of render_system.h)
2. `KeyboardShortcutSystem::Save` (lines 178–203 of input_system.h)
3. `app_frame()` SaveAs handler (lines 437–469 of main.cpp)

**New document** appears in both `handleMenuActionImpl` case 0 and `KeyboardShortcutSystem::New`.

**Bold toggle** appears in menu handler, keyboard shortcut system, and toolbar system.

## Proposed Solution

Create a `DocumentCommands` namespace (or class) in a new file `src/commands/document_commands.h` with free functions for each operation:

```cpp
namespace cmd {

void newDocument(ecs::DocumentComponent& doc, ecs::MenuComponent& menu);
void save(ecs::DocumentComponent& doc, ecs::LayoutComponent& layout);
void saveAs(ecs::DocumentComponent& doc, ecs::LayoutComponent& layout, const std::string& path);
void open(ecs::DocumentComponent& doc, ecs::LayoutComponent& layout, ecs::MenuComponent& menu, const std::string& path);
void toggleBold(ecs::DocumentComponent& doc);
void toggleItalic(ecs::DocumentComponent& doc);
void toggleUnderline(ecs::DocumentComponent& doc);
void undo(ecs::DocumentComponent& doc, ecs::CaretComponent& caret);
void redo(ecs::DocumentComponent& doc, ecs::CaretComponent& caret);
void cut(ecs::DocumentComponent& doc);
void copy(ecs::DocumentComponent& doc);
void paste(ecs::DocumentComponent& doc);
// ... etc for all shared operations

}
```

All three entry points (menu, keyboard, toolbar) would call the same function.

## Files to Change

| File | Change |
|------|--------|
| `src/commands/document_commands.h` | New file — all command implementations |
| `src/ecs/render_system.h` | `handleMenuActionImpl` delegates to `cmd::*` |
| `src/ecs/input_system.h` | `KeyboardShortcutSystem` delegates to `cmd::*` |
| `src/ecs/toolbar_system.h` | `ToolbarRenderSystem` delegates to `cmd::*` |
| `src/main.cpp` | `app_frame()` dialog handlers delegate to `cmd::*` |

## Risks

- Need to ensure toast notifications are still sent from the right context
- Some operations have slight variations (e.g., Save vs SaveAs) — the command layer should handle both
- Track changes revision recording must happen inside the command, not at the call site

## Migration Strategy

1. Start with the most duplicated operations: Save, New, Bold/Italic/Underline, Undo/Redo
2. Move one operation at a time, verifying E2E tests pass after each
3. Once all operations are migrated, `handleMenuActionImpl` becomes a thin dispatch table
