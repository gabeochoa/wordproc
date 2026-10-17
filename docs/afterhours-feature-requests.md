# Afterhours Migration & Feature Requests

Collected from the design audit (2026-02-08), UI migration work, and submodule upgrade planning.

**Current afterhours**: `d1d7f0e` (bumped 2026-10-17 from `17b982c`, 348 commits).
Status of every request below re-checked against that tree; see
`docs/plans/2026-08-16-afterhours-upgrade.md` for the August upgrade itself.
The October bump caused one breakage in `src/`: upstream's new
`src/crash_handler.h` calls `log_error()` on both sides of an `if`/`else`, and
wordproc's `log_*` were bare-`if` macros, so the trailing `else` had no
matching `if`. Fixed on the wordproc side — `src/log/log_macros.h` now expands
to a single `do { ... } while (0)` statement (with a regression test in
`tests/test_log_macros.cpp`). `AFTER_HOURS_REPLACE_LOGGING` documents `log_*`
as functions; the macros now at least behave like one statement.

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

**RESOLVED (2026-08-16)** — two ways: `ComponentConfig::with_icon(texture, source_rect)`
+ `with_icon_position()` for real textures, and `with_on_draw_bg/fg` for drawn icons.
wordproc took the second: each toolbar button carries its pixel-art icon as an
`on_draw_fg` callback, so the post-render overlay and its z-order workaround are gone.

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

**STILL OPEN** — re-checked at `d1d7f0e`: there is no mnemonic or per-character
underline API. `TextSpan` (`with_styled_label`) carries per-span colour and
`FontWeight`, and nothing else: no underline, no italic. Note the old
"workaround" line below was never true — wordproc draws no access-key
underlines at all today.

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

**STILL OPEN as config** — re-checked at `d1d7f0e`: no `with_hover_background` /
`with_hover_bevel` on `ComponentConfig`. wordproc keeps passing
`was_hot`/`is_mouse_inside` into its own button styling (`with_custom_hover_bg`
covers the fill case only).

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

**RESOLVED as drawable (2026-08-16)** — still no built-in dropdown arrow, but
`with_on_draw_fg` lets the trigger draw its own triangle inside the render pass.
wordproc does that now; `imm::dropdown_menu` remains an option for the widget itself.

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

**RESOLVED (2026-08-16)** — `RenderImm` draws a ring for `visual_focus_id`, themed by
`theme.focus` (`Theme::Usage::Focus`), `theme.focus_ring_thickness` and
`theme.focus_ring_offset` (0 thickness disables it; positive offset insets it).
wordproc sets 1px at 2px offset in black — see `src/ui/win95_theme.h`.
Dotted rings are still not a thing; the ring is solid.

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

**AVAILABLE, NOT ADOPTED — API REPLACED at `d1d7f0e`** — the old
`AnimationDef` / `AnimTrigger` / `AnimProperty` / `AnimCurve` /
`HasAnimationState` API is gone (upstream removed it); the replacement is
`src/plugins/animation/`: `Track<T>` over `Spring` / `Timeline` modes, keyed
`anim()` tracks stored on the entity, named spring presets, easing curves,
effect presets (`fade_up`, `pop_in`, `hover_lift`, …), `with_unit_motion`,
shader scopes and a pooled particle emitter. Still not wired into wordproc:
menu/toast motion changes E2E frame timing, and the E2E suite cannot be run on
a locked machine, so it needs a session with a display.

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

**RESOLVED UPSTREAM, NOT ADOPTED** — `d1d7f0e` brings `with_tooltip(text, delay)`
plus `with_tooltip_font_size` / `with_tooltip_padding` / `with_tooltip_gap`,
`UpdateTooltips` + `RenderTooltip` systems (already registered through
wordproc's existing `registerUIPostLayoutSystems` / `registerUIRenderSystems`
calls), hover delay, and edge-flipping placement via `overlay::place`.
Adoption deferred: it needs eyes on the result. The upstream tooltip renders
the theme's `raised_surface()` (near-white on the Win95 theme) with the
theme's `corner_radius` (default 8px → rounded), and has no background /
corner / border config, while wordproc's manual tooltip is pale yellow
(`255, 255, 225`), square, black-bordered, and instant. Adopting it as-is
would trade the Win95 look for the delay + edge-flipping behaviour.

**New gap:** tooltip has no background / foreground / corner / border config.

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

**RESOLVED (2026-08-16)** — `with_on_draw_bg(fn)` / `with_on_draw_fg(fn)`, where
`fn` is `std::function<void(RectangleType)>` receiving the widget's final rect and
running inside the render pass, so it obeys render layers. This is what killed
`ToolbarOverlayRenderSystem`.

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

**RESOLVED (2026-08-16)** — `with_shadow(Shadow{...})`, `with_hard_shadow(ox, oy, color)`
and `ShadowStyle::Hard|Soft`. wordproc puts a hard 2px shadow on the menu-bar
dropdowns and the formatting-bar lists.

**New gap (confirmed still open at `d1d7f0e`):** `imm::context_menu` /
`menu_list` build their panel with `ComponentConfig::inherit_from`, which
copies only inheritable fields and drops `shadow_config`, so a caller cannot
shadow the built-in menu. `apply_inheritable_from` still has no
`shadow_config` line in this pin.

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

**STILL OPEN** — re-checked at `d1d7f0e`: no scrollbar fields on `Theme` and no
`with_scrollbar`. Scroll views did gain smooth (eased) scrolling and off-screen
culling, but appearance is still internal.

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

**PARTIAL** — re-checked at `d1d7f0e`: `with_styled_label(std::vector<TextSpan>)`
renders per-span colour and `FontWeight` on one line, plus wrap-aware
measurement (`measure_text_wrapped`) and multi-line text that measures the same
on every backend. No italic, underline, strikethrough, super/subscript or
per-span size — so the document canvas stays custom, as predicted.

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

**PARTIAL at `d1d7f0e`** — upstream now has a real UI-layout grid:
`imm::grid(ctx, ep, GridConfig{}.with_rows(3).with_cols(4)
.with_cell_border(BorderWeight::Thin, color))` + `grid_cell(ctx, grid_elem, r, c, config)`
(+ `grid_row` / `grid_track`, col spans, per-column widths). But it lays out
*UI widgets* in `hstack` rows with a fixed `row_height` — it does not render
document tables. wordproc's document tables stay custom (`renderTable()` in
`render_system.h`): canvas text runs, borders, cell backgrounds and editing
highlights are not something a widget grid can express.

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

## 14. Right-Click / Multi-Button Mouse Support in E2E Testing

**RESOLVED (2026-10-17, at `d1d7f0e`)** — upstream now has the whole surface:
`test_input::simulate_right_click(x, y)` / `simulate_middle_click(x, y)`,
per-button `just_pressed` / `just_released` edges in `MouseState`
(`right_just_pressed`, `middle_just_pressed`, …) with multi-reader accessors,
`is_mouse_button_pressed(button)` returning per-button state in test mode, and
`right_click` / `middle_click` / `middle_down` / `middle_up` /
`right_click_ui` / `right_click_text` commands (the parser's fallthrough
splits `right_click x y` into two args even though bare `right_click` is not
in `coord_commands`). wordproc deleted its `HandleRightClickCommand` shim
(`src/testing/e2e_commands.h`): the builtin handler consumes `right_click`
first, and the menu system already polls `input::isMouseButtonPressed(1)`,
so a scripted right-click now opens the menu through the same path as a
physical one. Caveat: E2E cannot run on a locked machine, so the
`pass_context_menu_*.e2e` scripts exercising the real path are unverified
this session — run them on an unlocked machine.

**Need:** Simulate right-click (and potentially middle-click) in E2E tests, and expose right-click state through `is_mouse_button_pressed(1)` in test mode.

**Context:** Context menus are triggered by right-click. The E2E testing framework's `MouseState` only tracks left-click (`left_down`, `just_pressed`, `just_released`). In test mode, `is_mouse_button_pressed(button, ...)` returns `false` for any button != 0. There's no `simulate_right_click()` and no `right_click` E2E command.

**Current limitation:** `MouseState` in `input_injector.h` has a single set of press/release fields (implicitly left-button only). The `HandleClickCommand` calls `simulate_click()` which calls `simulate_mouse_press()` which sets `left_down`. No equivalent for button 1 (right) or button 2 (middle).

**What's needed:**
1. Extend `MouseState` to track per-button state (at least buttons 0-2):
   ```cpp
   struct MouseState {
     Position pos{};
     bool active = false;
     // Per-button state (0=left, 1=right, 2=middle)
     std::array<bool, 3> button_down{};
     std::array<bool, 3> button_just_pressed{};
     std::array<bool, 3> button_just_released{};
     std::array<int, 3> button_press_frames{};
     std::array<bool, 3> button_auto_release{};
   };
   ```
2. Add `simulate_right_click(x, y)` and `simulate_mouse_press(int button)` to `test_input.h`.
3. Update `is_mouse_button_pressed(button, ...)`, `is_mouse_button_down(button, ...)`, and `is_mouse_button_released(button, ...)` in `test_input.h` to return per-button state in test mode.
4. Update `reset_frame()` to clear per-button per-frame state.
5. Add `right_click` to `coord_commands` in `runner.h`.
6. Add `HandleRightClickCommand` in `command_handlers.h` (calls `simulate_right_click`).

**Former workaround (deleted 2026-10-17):** wordproc registered a custom E2E
command handler for `right_click` that bypassed input simulation and directly
set app-level context menu state. No longer needed — see RESOLVED above.

---

## 15. Context Menu UI Primitive

**RESOLVED (2026-08-16)** — `imm::context_menu(ctx, ep, items, at, open, config)` over
`imm::menu_list`, with `MenuItem{label, shortcut, separator, disabled}`,
`MenuItem::sep()`, a shortcut gutter, edge-flipping placement (`overlay::place`) and
dismiss-on-focus-loss. Also `dropdown_menu` and `popover`. Adopted in
`menu_ui_system.h`; the app still owns Escape, which the primitive does not handle.

**Need:** A reusable `context_menu()` immediate-mode component that opens a popup menu at arbitrary screen coordinates.

**Context:** Context menus (right-click menus) are a common UI pattern. They're visually identical to dropdown menus but triggered by right-click and positioned at the cursor. Currently no built-in support.

**Desired API:**
```cpp
struct ContextMenuItem {
    std::string label;
    std::string shortcut;  // display-only hint
    bool enabled = true;
    bool separator = false;
};

// Returns index of clicked item, or -1 if none.
// Sets open = false when an item is clicked or dismissed.
int context_menu(ctx, ep, bool &open, float x, float y,
                 std::span<const ContextMenuItem> items);
```

**Implementation notes:** A modal with `ClosedBy::Any`, no backdrop dim, no title bar, absolute position at `(x, y)` clamped to screen bounds. Inside: a `tray()` with one `button()` per item and `separator()` for dividers. Clicking any enabled item returns its index and closes.

**Workaround:** wordproc builds the context menu manually using existing `modal` + `tray` + `button` primitives. Works but requires ~50 lines of boilerplate that every app would duplicate.

---

## 16. Crash handler vs an asserting `log_error`

**STILL OPEN (new in this bump)** — upstream added `src/crash_handler.h`:
`report()` logs through `log_error`, including the running-system line and a
backtrace. That assumes `log_error` returns. wordproc's `log_error` asserts
(`assert(false)` after logging), and upstream's own validation docs say
"Strict mode: use log_error which will assert" — so a consumer with an
asserting `log_error` that installs the handler gets `report()` aborting on
its first line and re-entering the handler via `SIGABRT`. Upstream's own
`crash_handler_test.cpp` uses the stock non-asserting logger, so it cannot
catch this. wordproc does not install the handler, so no workaround is
needed — just do not call `afterhours::crash::install_handler()` until
upstream reports through a non-aborting path (e.g. the `log_sink_fn`).

## Priority

Updated after the 2026-10-17 bump: 1, 4, 5, 9, 10, 14 and 15 are done; 6 is
available (under a replaced API) but not adopted; 8 is resolved upstream but
not adopted (needs eyes — see its new-gap note); 12 and 13 are partial; 2, 3,
11 and 16 are untouched upstream.


| # | Feature | Impact | Effort | Notes |
|---|---------|--------|--------|-------|
| 1 | Toolbar icons/bitmaps | Critical — all 4 audits | Medium | Or use #9 custom render |
| 9 | Custom render callback | Critical — enables icons + more | Medium | Alternative to #1 |
| 2 | Access key underlines | Critical — accessibility | Medium | |
| 3 | Hover state | Major — all 4 audits | Low | |
| 5 | Focus indicators | Major — accessibility/WCAG | Medium | |
| 14 | Right-click mouse support | Major — enables context menus | Low | Resolved at `d1d7f0e`; shim deleted |
| 15 | Context menu primitive | Major — common UI pattern | Low | Workaround in wordproc |
| 10 | Drop shadows | Major — visual depth | Low | |
| 4 | Dropdown triangle | Major — visual polish | Low | |
| 8 | Tooltip component | Major — usability | Low | Upstream `with_tooltip` exists; adoption needs eyes |
| 11 | Scrollbar customization | Major — theme consistency | Medium | |
| 12 | Rich text / per-span styling | Minor — domain-specific | High | May stay as custom code |
| 13 | Table / grid layout | Minor — domain-specific | High | UI grid exists; doc tables stay custom |
| 7 | ~~Sans-serif font~~ | ~~Resolved~~ | — | Done |
| 6 | Animation/motion | Minor — polish | High | Old API replaced by Track/Timeline at `d1d7f0e` |
| 16 | Crash handler vs asserting log_error | Minor — latent | Low | Do not install handler until upstream fixes |
