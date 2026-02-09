# Afterhours Migration & Feature Requests

Collected from the design audit (2026-02-08), UI migration work, and submodule upgrade planning.

**Current afterhours**: `6b7ac9b` (wordproc) → `cca68c4` (latest, ~10 commits behind)

---

# Part A: Migration Tasks (no upstream changes needed)

These can be done now with the current or latest afterhours.

## M1. Update afterhours submodule ✅

Updated to `cca68c4` (10 commits: checkbox defaults refactor, validation system, resolution-independent sizing helpers, drawing primitives, scroll_view improvements, mutable render system signatures).

---

## M2. Remove `const_cast` in render systems ✅

Switched `EditorRenderSystem::for_each_with` and `MenuSystem::for_each_with` to mutable signatures. Zero `const_cast` remaining in `src/`.

---

## M3. Replace direct raylib draw calls with afterhours wrappers ✅

All ~111 `raylib::Draw*` calls replaced with `afterhours::draw_*` equivalents:
- `src/util/drawing.h` — 4 calls migrated
- `src/ui/theme.h` — 2 calls migrated
- `src/ecs/render_system.h` — 52 calls migrated
- `src/ecs/toolbar_overlay_render.h` — 47 calls migrated
- `src/renderer/` — deleted (dead code, never used)

---

## M4. Mark AfterhoursGaps docs as resolved ✅

Deleted entire `AfterhoursGaps/` directory (~15 files). All information consolidated into this document.

---

## M5. Adopt afterhours CommandHistory\<T\> ✅ (already done)

`text_buffer.h` already uses `afterhours::CommandHistory<TextBuffer>`. No work needed.

---

## M6. Use afterhours text_input utilities ✅ (already done)

`text_buffer.cpp` already uses `afterhours::text_input::find_word_start` and `find_word_end`. No work needed.

---

## M7. Enable validation ✅

Enabled `enable_development_validation()` in `main.cpp` after `initUIContext`. Registered all validation systems (`register_systems<InputAction>`) after render systems. Logs warnings for off-screen elements, poor contrast, tiny fonts, etc.

---

## Migration Status

All 7 migration tasks are **complete** ✅. Summary:

| Task | Status | Notes |
|------|--------|-------|
| M1 — Update submodule | ✅ | `6b7ac9b` → `cca68c4` |
| M2 — Remove const_cast | ✅ | 0 const_cast in src/ |
| M3 — Replace raylib draws | ✅ | 0 `raylib::Draw*` in src/ |
| M4 — Clean up gap docs | ✅ | `AfterhoursGaps/` deleted |
| M5 — CommandHistory | ✅ | Already adopted |
| M6 — Text utils | ✅ | Already adopted |
| M7 — Validation | ✅ | Development mode enabled |

---

# Part B: Feature Requests (need upstream afterhours changes)

These items cannot be implemented locally without changes to the afterhours library.

---

## 1. Toolbar Icons / Bitmap Rendering

**Need:** Render 16x16 pixel-art icons inside toolbar buttons instead of text labels.

**Context:** All four design audits (Win95, Apple HIG, Sun JLF, Material Design 3) flagged single-letter toolbar labels (N, O, S, P, X, C, V, <, >) as a critical issue. Win95 requires recognizable graphic images (page=New, folder=Open, floppy=Save, etc.).

**Current limitation:** `ComponentConfig::with_label()` only supports text. No `.with_icon()` or `.with_texture()` API exists for rendering bitmaps inside buttons.

**Desired API:**
```cpp
button(ctx, mk(uiRoot, id),
    ComponentConfig{}
        .with_icon("toolbar_new", 16, 16)  // texture name, w, h
        .with_size(ComponentSize{pixels(24), pixels(22)})
        // ...
```

**Workaround:** Simple geometric pixel-art icons drawn as raylib primitives in `ToolbarOverlayRenderSystem` (post-render overlay). Icons include: blank page (New), folder (Open), floppy disk (Save), printer, scissors (Cut), overlapping pages (Copy), clipboard (Paste), curved arrows (Undo/Redo), horizontal lines (Align L/C/R/J).

---

## 2. Per-Character Text Decoration (Access Keys / Mnemonics)

**Need:** Render a single underlined character within a label (e.g., "**F**ile" with F underlined).

**Context:** Win95 and Sun JLF require access key underlines on every menu title and menu item. This is a critical accessibility feature for keyboard-only users.

**Current limitation:** `with_label()` renders all characters with the same style. No support for inline formatting like underline on specific characters.

**Desired API:**
```cpp
// Option A: Ampersand convention (like Win32)
.with_label("&File")  // renders "File" with F underlined

// Option B: Explicit mnemonic
.with_label("File")
.with_mnemonic('F')   // underlines the F
```

**Workaround:** Underlines drawn via raylib `DrawLine` in `ToolbarOverlayRenderSystem` after afterhours UI render. Measures text position to calculate underline placement under the first character of each menu header label.

---

## 3. Hover State / Mouse-Over Callback

**Need:** Change button appearance on mouse hover (before click).

**Context:** Win95 and JLF both specify hover effects on toolbar buttons. Win95 shows raised border on hover; JLF shows border only on hover (flat otherwise). Material Design 3 requires 7 distinct interactive states including hover.

**Current limitation:** `is_hot()` exists on `UIContext` and can be queried after a button is created, but `ComponentConfig` has no `.with_hover_background()` or hover state config. Changing appearance based on hover requires manual state tracking each frame.

**Desired API:**
```cpp
.with_hover_background(hoverColor)
.with_hover_bevel(BevelStyle::Raised, light, dark, width)
```

**Workaround:** Uses `ctx.was_hot(id)` to check previous-frame hover state and pass it to `absToolbarButton()`. Buttons are flat (no bevel) by default, show raised bevel on hover, and sunken bevel when pressed. Matches Win95 Office 97+ toolbar convention.

---

## 4. Dropdown Triangle Glyph (▼)

**Need:** Dropdown buttons should render a proper ▼ triangle instead of appending " v" to the label text.

**Context:** The formatting bar dropdowns ("Normal v", "Times New Roman v", "10 v") use a lowercase "v" as the dropdown indicator. The Unicode ▼ (U+25BC) doesn't render because the loaded font (EB Garamond) doesn't include it.

**Options:**
1. Load a font that includes ▼ (increases startup time)
2. Afterhours draws the triangle programmatically as part of dropdown rendering
3. Afterhours provides a built-in dropdown component that auto-renders the arrow

**Desired API:**
```cpp
// Option: Built-in dropdown with arrow
dropdown(ctx, mk(uiRoot, id),
    DropdownConfig{}
        .with_selected_label("Normal")
        .with_arrow_style(ArrowStyle::FilledTriangle)
```

**Workaround:** Filled triangle drawn programmatically via `raylib::DrawTriangle` in `ToolbarOverlayRenderSystem`. The " v" text suffix was removed from dropdown labels. Triangle positions are stored during update phase and drawn in the post-render overlay.

---

## 5. Focus Indicators / Keyboard Focus Ring

**Need:** Visible keyboard focus rectangles on interactive elements.

**Context:** WCAG 2.4.7 requires visible focus indicators. Win95 uses dotted focus rectangles. All four audits flagged missing focus indicators.

**Current state:** Afterhours has `HandleTabbing` and `ComputeVisualFocusId` systems registered, but no visible focus ring is rendered by `RenderImm`.

**Questions:**
- Does `RenderImm` draw a focus ring when `visual_focus_id` matches an entity?
- If not, can we hook into the focus system to draw our own?
- What is the intended API for focus ring styling?

**Desired API:**
```cpp
// Theme-level
theme.set_focus_ring(FocusRingStyle::Dotted, color, width);

// Or per-component
.with_focus_ring(FocusRingStyle::Dotted, color, width)
```

**Workaround:** None. Focus indicators are invisible.

---

## 6. Animation / Motion Transitions

**Need:** Subtle open/close animations on menus, toast slide-in/fade-out, button press feedback.

**Context:** Material Design 3 emphasizes physics-based spring animations. Even Win95 had subtle menu animations. The current UI has no motion — everything appears/disappears instantly.

**Current limitation:** Afterhours immediate-mode UI rebuilds every frame, so CSS-style transitions aren't directly applicable.

**Desired API:**
```cpp
// Spring-based transition on a property
.with_transition(Property::Opacity, SpringConfig{stiffness, damping})
.with_transition(Property::TranslateY, SpringConfig{stiffness, damping})

// Or a transition system
afterhours::transition::animate(entityId, "opacity", 0.0f, 1.0f, 200ms);
```

**Workaround:** None. Menus, toasts, and state changes are instant.

---

## 7. ~~Sans-Serif UI Font~~ (RESOLVED)

**Resolved:** Loaded Roboto-Regular.ttf as the UI font. Afterhours `FontManager::DEFAULT_FONT` and `theme::UI_FONT` now use Roboto. Document text uses its own font rendering path (unaffected). No afterhours changes needed.

---

## 8. Tooltip Component

**Need:** Built-in tooltip support with delay, positioning, and auto-dismiss.

**Context:** Win95 requires tooltips on every toolbar button. Currently implemented as a manual hover-check + div rendering, but it doesn't support hover delay, proper z-ordering, or edge-of-screen repositioning.

**Current workaround:** Manual `is_hot()` check + absolute-positioned div with `render_layer(20)`. Works but has no delay and positioning is fixed.

**Desired API:**
```cpp
.with_tooltip("New Document (Ctrl+N)")
.with_tooltip_delay(700)  // ms before showing
```

---

## 9. Custom Render Callback

**Need:** Draw arbitrary content (geometric primitives, charts, custom graphics) inside a UI component's bounds.

**Context:** Toolbar icons are 16x16 pixel-art drawn from lines, triangles, and rectangles. Currently drawn as a raw raylib overlay AFTER the afterhours render pass, which causes z-order issues (icons appear on top of menus/dropdowns). An alternative to pre-rendered textures (#1) would be letting components define a custom draw function that runs during the normal render pass, respecting render layers.

**Current limitation:** `ComponentConfig` supports `with_label()` (text) and `with_texture()` (images) but no way to inject arbitrary drawing code that participates in the render layer system.

**Desired API:**
```cpp
button(ctx, mk(uiRoot, id),
    ComponentConfig{}
        .with_size(ComponentSize{pixels(24), pixels(24)})
        .with_custom_render([](const Rect& bounds, const Theme& theme) {
            // Draw icon primitives within bounds
            draw_line(bounds.x + 4, bounds.y + 4, bounds.x + 20, bounds.y + 4, color);
            // ...
        })
```

**Workaround:** `ToolbarOverlayRenderSystem` draws icons after the afterhours render pass. When menus or dropdowns are open, the overlay system skips drawing entirely to avoid z-order conflicts (icons disappear instead of rendering behind the popup).

---

## 10. Drop Shadows on Popup Elements

**Need:** Menu dropdowns and popup panels should cast a drop shadow to visually float above the content beneath them.

**Context:** Win95 menus have a 2px shadow on the right and bottom edges. Material Design 3 uses elevation shadows extensively. Currently, menus appear flat against the toolbar/content with no visual separation.

**Current limitation:** `ComponentConfig` has no shadow support. `with_bevel()` provides 3D borders but not offset shadows.

**Desired API:**
```cpp
.with_shadow(ShadowConfig{
    .offset_x = 2, .offset_y = 2,
    .blur = 0,          // Win95: hard shadow. MD3: blurred shadow
    .color = {0, 0, 0, 80}
})
```

**Workaround:** None. Menus have no shadow.

---

## 11. Scrollbar Visual Customization

**Need:** Style the scrollbar thumb/track to match the Win95 theme (raised 3D thumb in a sunken track).

**Context:** `HasScrollView` provides scroll behavior and clipping, but the rendered scrollbar appearance is controlled by afterhours internally. The word processor needs a Win95-style scrollbar: gray sunken track, raised button-face thumb with light/dark 3D borders, and up/down arrow buttons at the ends.

**Current limitation:** No API to customize scrollbar colors, bevel style, width, or arrow buttons. Unclear whether `HasScrollView` even renders a visible scrollbar or just provides scroll behavior.

**Desired API:**
```cpp
// Theme-level scrollbar styling
theme.scrollbar_width = 16.0f;
theme.scrollbar_track_color = BUTTON_FACE;
theme.scrollbar_thumb_bevel = BevelStyle::Raised;
theme.scrollbar_arrow_buttons = true;

// Or per-component
.with_scrollbar(ScrollbarStyle{
    .width = 16, .track = sunkenStyle, .thumb = raisedStyle
})
```

**Workaround:** Scrollbar drawn manually with raw raylib in `EditorRenderSystem` (~50 lines). Scroll position managed by a custom `ScrollComponent`, not connected to afterhours `HasScrollView`.

---

## 12. Rich Text / Per-Span Styled Text

**Need:** Render text with different styles (bold, italic, color, size) within a single text element.

**Context:** The document canvas renders text with per-line paragraph styles (headings, titles), bold/italic simulation, underline, strikethrough, superscript/subscript, drop caps, text colors, and highlight backgrounds. This is the core of a word processor. afterhours' `text_area.h` handles plain text input but not styled/rich text rendering.

**Current limitation:** `with_label()` renders uniform text. No support for inline style spans like "Hello **bold** world" or per-character colors.

**Desired API:**
```cpp
// Option A: Markup-style
.with_rich_label("<b>Hello</b> <i>world</i>")

// Option B: Span-based
.with_text_spans({
    {.text = "Hello ", .bold = true, .color = black},
    {.text = "world", .italic = true, .color = gray},
})
```

**Assessment:** This is a large feature that may not belong in afterhours (it's domain-specific to editors/word processors). The document text renderer will likely stay as custom raylib drawing. Listed here for completeness.

**Workaround:** `renderTextBuffer()` in `render_system.h` (~280 lines) uses raw raylib `DrawText` with manual bold simulation (draw twice with 1px offset), manual underline/strikethrough (`DrawLine`), and manual selection highlighting (`DrawRectangle`).

---

## 13. Table / Grid Layout Component

**Need:** Render a grid of cells with configurable borders (none/thin/medium/thick), per-cell backgrounds, and text content.

**Context:** The word processor supports inserted tables with customizable borders and cell styling. Currently rendered with ~100 lines of raw raylib drawing (nested loops over rows/cols, border style switches, cell background fills).

**Current limitation:** afterhours autolayout supports flex-based layouts but not a true CSS Grid-style layout with explicit rows/columns, cell spanning, or per-cell border control.

**Desired API:**
```cpp
auto tbl = grid(ctx, mk(uiRoot, id),
    GridConfig{}
        .with_rows(3).with_cols(4)
        .with_cell_border(BorderStyle::Thin, borderColor)
);
for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 4; c++) {
        grid_cell(ctx, mk(uiRoot, cellId),
            CellConfig{}.with_label(content).with_background(bgColor));
    }
}
```

**Assessment:** Like rich text, this is fairly domain-specific. A general grid/table layout would be useful but is a significant feature. Listed for completeness.

**Workaround:** `renderTable()` in `render_system.h` (~100 lines) draws cells, borders, text, and editing highlights with raw raylib.

---

## Priority

| # | Feature | Impact | Effort | Notes |
|---|---------|--------|--------|-------|
| 1 | Toolbar icons/bitmaps | Critical — all 4 audits | Medium | Or use #9 custom render |
| 9 | Custom render callback | Critical — enables icons + more | Medium | Alternative to #1 |
| 2 | Access key underlines | Critical — accessibility | Medium | |
| 3 | Hover state | Major — all 4 audits | Low | |
| 5 | Focus indicators | Major — accessibility/WCAG | Medium | |
| 10 | Drop shadows | Major — visual depth | Low | |
| 4 | Dropdown triangle | Major — visual polish | Low | |
| 8 | Tooltip component | Major — usability | Low | |
| 11 | Scrollbar customization | Major — theme consistency | Medium | |
| 12 | Rich text / per-span styling | Minor — domain-specific | High | May stay as custom code |
| 13 | Table / grid layout | Minor — domain-specific | High | May stay as custom code |
| 7 | ~~Sans-serif font~~ | ~~Resolved~~ | — | Done |
| 6 | Animation/motion | Minor — polish | High | |
