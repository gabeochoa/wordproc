# Afterhours Feature Requests

Collected from the design audit (2026-02-08) and UI migration work.
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

## Priority

| # | Feature | Impact | Effort |
|---|---------|--------|--------|
| 1 | Toolbar icons/bitmaps | Critical — all 4 audits | Medium |
| 2 | Access key underlines | Critical — accessibility | Medium |
| 3 | Hover state | Major — all 4 audits | Low |
| 5 | Focus indicators | Major — accessibility/WCAG | Medium |
| 4 | Dropdown triangle | Major — visual polish | Low |
| 8 | Tooltip component | Major — usability | Low |
| 7 | Sans-serif font | Minor — visual fidelity | Low |
| 6 | Animation/motion | Minor — polish | High |
