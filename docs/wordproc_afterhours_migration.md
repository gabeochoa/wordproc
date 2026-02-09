# Wordproc Afterhours Migration Checklist

**Date**: 2026-02-07
**Current afterhours**: `6b7ac9b` (wordproc) → `cca68c4` (latest, 10 commits behind)

Most items in `AfterhoursGaps/` are now resolved upstream. This doc tracks what wordproc needs to do to catch up.

---

## 1. Update afterhours submodule

```bash
cd vendor/afterhours && git pull origin main
```

This picks up: checkbox defaults refactor, validation system, resolution-independent sizing helpers, drawing primitives, scroll_view improvements, and more.

---

## 2. Remove `const_cast` in render systems (RESOLVED UPSTREAM)

Afterhours render systems now call non-const `for_each_with` first. Wordproc can drop `const_cast` and use mutable signatures directly.

**Files to change:**

- `src/ecs/render_system.h` (lines 620-822) — 6 `const_cast` uses
  - `MenuSystem::for_each_with` — switch from `const` to mutable override
  - Help window render — switch from `const` to mutable override

**Before:**
```cpp
void for_each_with(const Entity& entity,
                   const DocumentComponent& docConst,
                   const MenuComponent& menuConst,
                   const LayoutComponent& layoutConst,
                   const float) const override {
    auto& doc = const_cast<DocumentComponent&>(docConst);
    auto& menu = const_cast<MenuComponent&>(menuConst);
    auto& layout = const_cast<LayoutComponent&>(layoutConst);
    renderMenus(doc, menu, layout);
}
```

**After:**
```cpp
void for_each_with(Entity& entity,
                   DocumentComponent& doc,
                   MenuComponent& menu,
                   LayoutComponent& layout,
                   float) override {
    renderMenus(doc, menu, layout);
}
```

**Effort:** ~15 minutes. Just change signatures, remove casts.

---

## 3. Replace direct raylib draw calls with afterhours wrappers

Afterhours now has `draw_line()`, `draw_circle()`, `draw_ellipse()`, `draw_triangle()`, `draw_poly()`, etc. in `drawing_helpers.h`. This keeps the abstraction clean for future backend swaps.

**Files with direct raylib draw calls (~111 total):**

| File | Direct raylib calls | Notes |
|------|-------------------|-------|
| `src/util/drawing.h` | 4 `raylib::DrawLine` | Sunken border helper — use `afterhours::draw_line()` |
| `src/ui/theme.h` | 2 `raylib::DrawText*` | `DrawUIText` helper — use `afterhours::draw_text_ex()` |
| `src/ecs/render_system.h` | ~52 calls | Main document renderer — largest migration |
| `src/ecs/toolbar_overlay_render.h` | ~47 calls | Toolbar tooltip/overlay rendering |
| `src/renderer/raylib_renderer.h` | ~6 calls | Low-level renderer |

**Effort:** ~1-2 hours. Mechanical find-and-replace. Start with `drawing.h` (smallest) as a test.

---

## 4. Mark AfterhoursGaps docs as resolved

These gap docs are now stale and should be updated:

| Gap | Status | Action |
|-----|--------|--------|
| 03 - Text editing | Already marked ✅ | No change needed |
| 05 - Render const constraint | **RESOLVED** | Mark ✅ — render systems are now mutable |
| 08 - Scrollable containers | **RESOLVED** | Mark ✅ — `scroll_view()`, `HasScrollView`, scissor, mouse wheel all exist |
| 09 - Modal dialogs | Already marked ✅ | No change needed |
| 10 - Command history | **RESOLVED** | Mark ✅ — `plugins/command_history.h` with `Command<T>` exists |
| 11 - Status notifications | Already marked ✅ | No change needed |
| 14 - Uninitialized size | **Likely resolved** | Check if warning exists in rendering code |
| 16 - Drawing tools | **Primitives resolved** | Mark primitives ✅; line styles/shape editing are app-level |
| 17 - Pluggable backends | Future/aspirational | Leave as-is |

---

## 5. Optional: Adopt afterhours CommandHistory<T>

Wordproc has its own `CommandHistory` class in `src/editor/text_buffer.h`. Afterhours provides a generic `CommandHistory<T>` in `plugins/command_history.h` with the same API plus command merging.

**Migration:**
- Change `EditCommand` → `afterhours::Command<TextBuffer>`
- Change `CommandHistory` → `afterhours::CommandHistory<TextBuffer>`
- Gain: command merging support (e.g., merge sequential character inserts)

**Effort:** ~30 minutes. Low risk, high alignment.

---

## 6. Optional: Use afterhours text_input utilities

Wordproc has custom UTF-8 and word navigation code. Afterhours provides these in `text_input/utils.h`:
- `utf8_char_length()`, `utf8_prev_char_start()`, `codepoint_to_utf8()`
- `find_word_start()`, `find_word_end()`, `select_word_at()`

Low priority — wordproc's implementations work fine. Adopt incrementally to reduce code duplication.

---

## 7. Optional: Enable validation

Cartographer already uses `ValidationConfig` with `ValidationMode::Warn`. Wordproc could enable this for catching layout issues early (screen bounds, contrast, min font size, etc.).

```cpp
auto& styling = afterhours::ui::imm::UIStylingDefaults::get();
ValidationConfig& vc = styling.get_validation_config_mut();
vc.mode = ValidationMode::Warn;
vc.enforce_screen_bounds = true;
vc.enforce_min_font_size = true;
```

---

## Priority Order

1. **Update submodule** — unlocks everything else
2. **Remove const_cast** — eliminates undefined behavior
3. **Replace raylib draw calls** — maintains abstraction
4. **Mark gaps resolved** — reduces confusion
5. **CommandHistory migration** — nice-to-have
6. **Text utils adoption** — nice-to-have
7. **Enable validation** — nice-to-have

---

## Not Needed

- **Icon registry in afterhours** — wordproc manages icons fine at app level
- **Docked layout helper** — absolute positioning works for Win95 chrome, not worth vendor work
- **Sokol backend** — future aspiration, not blocking anything
