# Design Audit Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement the 32 "Yes — Doing" items from the design audit triage in `docs/plans/2026-02-08-design-audit-todos.md`.

**Architecture:** Each task modifies existing ECS systems (toolbar, menu, status bar, title bar, render) and data files (menu_setup, theme). No new ECS systems needed. Afterhours-dependent items are deferred to the end with workaround docs.

**Tech Stack:** C++, afterhours ECS, raylib (via afterhours wrappers), immediate-mode UI

---

## File Reference

| File | Role |
|------|------|
| `src/ecs/toolbar_system.h` | Standard + formatting toolbar rendering (buttons, dropdowns) |
| `src/ecs/menu_ui_system.h` | Menu bar + dropdown rendering, menu action dispatch |
| `src/ecs/status_bar_system.h` | Word 6.0-style status bar |
| `src/ecs/title_bar_system.h` | Title bar (blue, white text) |
| `src/ecs/render_system.h` | Document area rendering, ruler, scroll, menu action handling |
| `src/ecs/components.h` | ECS component definitions |
| `src/ui/menu_setup.h` | Menu bar data (labels, shortcuts, structure) |
| `src/ui/theme.h` | Colors, layout constants, font helpers |
| `src/ui/ui_context.h` | Afterhours UI setup, toast helpers |
| `src/ui/ah_win95_widgets.h` | Win95-styled afterhours component configs |

## Build & Test Commands

```bash
make all          # Build (must pass with 0 errors)
make run          # Run the app (visual check)
make test         # Unit tests
make e2e          # E2E tests with screenshots
```

---

## Phase 1: Quick Wins — Data & Text Changes (no afterhours changes needed)

### Task 1: Menu order + Help last (#5)

Reorder menus in `menu_setup.h` to: File, Edit, View, Insert, Format, Tools, Table, Help. Merge Settings items into Tools.

**Files:**
- Modify: `src/ui/menu_setup.h`

**Step 1: Reorder menus in `createMenuBar()`**

The current order is: File, Edit, View, Format, Insert, Table, Help, Tools, Settings.
Change to: File, Edit, View, Insert, Format, Tools, Table, Help.

Move the `insertMenu` block above `formatMenu`. Move `toolsMenu` before `tableMenu`. Add Settings items ("UI Scale...", "Preferences...") to the end of `toolsMenu.items`. Delete the `settingsMenu` block entirely.

```cpp
// After Edit menu:
menus.push_back(editMenu);  // Edit
menus.push_back(viewMenu);  // View
menus.push_back(insertMenu); // Insert  (was after Format)
menus.push_back(formatMenu); // Format  (was after View)
menus.push_back(toolsMenu);  // Tools   (was after Help)
menus.push_back(tableMenu);  // Table   (was after Insert)
menus.push_back(helpMenu);   // Help    (always last)
// DELETE settingsMenu entirely
```

Add to `toolsMenu.items` before pushing:
```cpp
toolsMenu.items.push_back({"", "", false, true, nullptr});  // Separator
toolsMenu.items.push_back({"UI Scale...", "", true, false, nullptr});
toolsMenu.items.push_back({"Preferences...", "", false, false, nullptr}); // disabled
```

**Step 2: Update menu index references in `render_system.h`**

The menu action handler in `EditorRenderSystem::for_each_with_ref` uses `menu.activeMenuIndex` to dispatch actions. After reordering, the indices change:

| Menu | Old Index | New Index |
|------|-----------|-----------|
| File | 0 | 0 |
| Edit | 1 | 1 |
| View | 2 | 2 |
| Format | 3 | 4 |
| Insert | 4 | 3 |
| Table | 5 | 6 |
| Help | 6 | 7 |
| Tools | 7 | 5 |
| Settings | 8 | DELETED |

Search `render_system.h` for `menu.activeMenuIndex ==` and update all case values. Also handle the old Settings cases (UI Scale) — move those into the Tools handler.

**Step 3: Build and run**

```bash
make all   # Must compile clean
make run   # Visual: verify menus appear in correct order, Help is rightmost
```

**Step 4: Run e2e tests**

```bash
make e2e   # Must pass — menu_select commands reference menu labels not indices
```

**Step 5: Commit**

```bash
git add -A && git commit -m "reorder menus to standard: File Edit View Insert Format Tools Table Help"
```

---

### Task 2: Keyboard shortcuts on all menus (#10)

Many menu items already have shortcuts in `menu_setup.h` but some common ones are missing. Add shortcut annotations to items that have working keyboard bindings.

**Files:**
- Modify: `src/ui/menu_setup.h`

**Step 1: Add missing shortcut strings**

In `createMenuBar()`, update these items to show their shortcuts:

```cpp
// Edit menu — these already have bindings in input_system.h
{"Cut", "Ctrl+X", ...}       // already has it
{"Copy", "Ctrl+C", ...}      // already has it  
{"Paste", "Ctrl+V", ...}     // already has it

// Format menu — add shortcuts that are bound
{"Bold", "Ctrl+B", ...}              // already has it
{"Italic", "Ctrl+I", ...}            // already has it
{"Underline", "Ctrl+U", ...}         // already has it
```

Check `input_system.h` for all bound shortcuts and verify they match the annotations. Any item with a working key binding should show it.

**Step 2: Build and verify**

```bash
make all && make run  # Check that shortcuts appear right-aligned in menus
```

**Step 3: Commit**

```bash
git add -A && git commit -m "show keyboard shortcuts on all menu items that have bindings"
```

---

### Task 3: Status bar green text → normal color (#9)

**Files:**
- Modify: `src/ecs/render_system.h` (auto-save notification)
- Modify: `src/ui/ui_context.h` (if toast colors are set here)

**Step 1: Find auto-save toast calls**

Search for `toast_notify::` calls and the auto-save handler. The auto-save happens in `render_system.h` inside `AutoSaveSystem` or in `input_system.h`. Change:

```cpp
// BEFORE: green toast
toast_notify::success("Auto-saved");
// AFTER: plain info toast  
toast_notify::info("Auto-saved", 2.0f);
```

Also find any direct green-text status bar drawing. In `status_bar_system.h`, the status text already uses `win95_colors::TEXT` (black) — that's correct. The problem is the toast system using green.

**Step 2: Build and run**

```bash
make all && make run  # Trigger auto-save, verify toast is blue/neutral not green
```

**Step 3: Commit**

```bash
git add -A && git commit -m "change auto-saved notification from green to neutral info style"
```

---

### Task 4: Toast notification stacking — cap at 1-2 (#3)

**Files:**
- Modify: `src/ui/ui_context.h` (toast config)
- Check: `vendor/afterhours/src/plugins/toast.h` for max-toast config

**Step 1: Check afterhours toast API for max count**

Read `vendor/afterhours/src/plugins/toast.h` and look for a max toast count setting or a way to limit visible toasts. If afterhours supports `max_toasts` or similar config, use it.

If afterhours does NOT support limiting toasts, create a workaround: before sending a new toast, clear existing ones. OR reduce the toast duration to 2 seconds so they expire fast.

**Step 2: Implement the fix**

Option A — If afterhours has config:
```cpp
// In ui_context.h or main.cpp during setup
auto* toastConfig = afterhours::EntityHelper::get_singleton_cmp<afterhours::toast::ToastConfig>();
if (toastConfig) {
    toastConfig->max_visible = 2;
}
```

Option B — Workaround (reduce duration):
```cpp
// Change all toast_notify::info() calls to use 2.0f duration
toast_notify::info("Auto-saved", 2.0f);
```

Option C — Replace auto-save toast with status bar message:
In `render_system.h` auto-save handler, instead of `toast_notify::info("Auto-saved")`, set a transient status bar message that the `StatusBarSystem` can display.

**Step 3: Build, run, trigger auto-save, verify max 1-2 toasts**

```bash
make all && make run
```

**Step 4: Commit**

```bash
git add -A && git commit -m "cap toast notifications at 1-2 visible, reduce auto-save toast spam"
```

---

### Task 5: Color-only status feedback (#11)

Change auto-save toast to include an icon prefix so it's not color-dependent.

**Files:**
- Modify: `src/ecs/render_system.h` (auto-save toast calls)

**Step 1: Update toast messages**

Already partially done in Task 3. Ensure all `toast_notify::info()` calls use descriptive text:

```cpp
// BEFORE
toast_notify::info("Auto-saved", 2.0f);
// AFTER — icon prefix makes meaning clear without color
toast_notify::info("[saved] Auto-saved", 2.0f);
```

For other toast types, ensure the message text is self-explanatory:
```cpp
toast_notify::error("[error] Save failed: " + result.error);
toast_notify::warning("[!] Not found");
toast_notify::success("[ok] Opened: " + filename);
```

**Step 2: Build and verify**

```bash
make all && make run
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add text prefixes to toast notifications for non-color feedback"
```

---

### Task 6: Insert menu — add cascading submenu for shapes (#19)

Move shape items (Line, Rectangle, Circle, Ellipse, Arrow, Rounded Rectangle, Triangle) under a single "Shape" parent item in the Insert menu.

**Files:**
- Modify: `src/ui/menu_setup.h`
- Modify: `src/ecs/render_system.h` (Insert menu action handler — update indices)

**Step 1: Check if afterhours/menu system supports submenus**

Look at `win95::MenuItem` in `src/ui/win95_widgets.h` — does it support `children` or `submenu`? If not, the simplest approach is to remove the individual shape items and keep only "Shape..." which opens a dialog. This reduces the Insert menu from 30+ items to ~20.

```cpp
// BEFORE (7 separate items):
{"Shape...", "", true, false, nullptr},
{"Line", "", true, false, nullptr},
{"Rectangle", "", true, false, nullptr},
// ... etc

// AFTER (single item):
{"Shape...", "", true, false, nullptr},  // Opens shape picker dialog
// DELETE Line, Rectangle, Circle, Ellipse, Arrow, Rounded Rectangle, Triangle
```

**Step 2: Update Insert menu action handler indices in `render_system.h`**

After removing 7 items, all indices after "Shape..." shift down by 7. Update the `case` values in the Insert menu handler.

**Step 3: Build and test**

```bash
make all && make run   # Verify Insert menu is shorter
make e2e               # E2E tests that use Insert menu items
```

**Step 4: Commit**

```bash
git add -A && git commit -m "consolidate shape items into single Shape menu entry, shorten Insert menu"
```

---

### Task 7: Insert menu disabled states (#25)

Gray out menu items that don't apply in the current context.

**Files:**
- Modify: `src/ecs/menu_ui_system.h` or `src/ecs/render_system.h`

**Step 1: Add dynamic enable/disable logic**

Before rendering menus or in the menu update system, set `.enabled = false` on items that aren't applicable:

```cpp
// In MenuUISystem or render_system.h, before drawing menus:
// "Remove Hyperlink" — only enabled if cursor is on a hyperlink
// "Paste" — only enabled if clipboard has content
// "Cut"/"Copy" — only enabled if there's a selection
// "Undo" — only enabled if canUndo()
// "Redo" — only enabled if canRedo()

// Example for Edit menu:
auto& editMenu = menu.menus[1]; // Edit is index 1
editMenu.items[0].enabled = doc.buffer.canUndo();  // Undo
editMenu.items[1].enabled = doc.buffer.canRedo();  // Redo
editMenu.items[7].enabled = doc.buffer.hasSelection(); // Cut
editMenu.items[8].enabled = doc.buffer.hasSelection(); // Copy
// Insert > Remove Hyperlink
auto& insertMenu = menu.menus[3]; // Insert is now index 3
insertMenu.items[4].enabled = false; // Remove Hyperlink — enable only when on hyperlink
```

**Step 2: Build and run**

```bash
make all && make run   # Open menus, verify disabled items are grayed
```

**Step 3: Commit**

```bash
git add -A && git commit -m "gray out menu items that dont apply in current context"
```

---

## Phase 2: Title Bar & Window Controls

### Task 8: Window control buttons (#13)

Add minimize, maximize, close buttons to the right side of the title bar.

**Files:**
- Modify: `src/ecs/title_bar_system.h`

**Step 1: Add window control buttons**

After the title bar div, add three small buttons (minimize, maximize, close) positioned absolutely on the right side:

```cpp
// Window control button dimensions
float btnW = theme::layout::scale(16);
float btnH = theme::layout::scale(14);
float btnPad = theme::layout::scale(2);
float btnY = (titleBarHeight - btnH) / 2.0f;

// Close button (rightmost)
float closeX = screenWidth - btnW - btnPad;
if (button(ctx, mk(uiRoot, 9010),
    ComponentConfig{}
        .with_label("X")
        .with_size(ComponentSize{pixels(btnW), pixels(btnH)})
        .with_absolute_position()
        .with_translate(closeX, btnY)
        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_bevel(afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_roundness(0.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center)
        .with_debug_name("btn_close"))) {
    // Close the application
    raylib::CloseWindow();
}

// Maximize button
float maxX = closeX - btnW - btnPad;
if (button(ctx, mk(uiRoot, 9011),
    ComponentConfig{}
        .with_label("□")  // or use a simple square glyph
        .with_size(ComponentSize{pixels(btnW), pixels(btnH)})
        .with_absolute_position()
        .with_translate(maxX, btnY)
        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_bevel(afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_roundness(0.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center)
        .with_debug_name("btn_maximize"))) {
    // Toggle maximize (raylib doesn't natively support this easily)
}

// Minimize button
float minX = maxX - btnW - btnPad;
if (button(ctx, mk(uiRoot, 9012),
    ComponentConfig{}
        .with_label("_")
        .with_size(ComponentSize{pixels(btnW), pixels(btnH)})
        .with_absolute_position()
        .with_translate(minX, btnY)
        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_bevel(afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_roundness(0.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center)
        .with_debug_name("btn_minimize"))) {
    raylib::MinimizeWindow();
}
```

**Step 2: Build and run**

```bash
make all && make run   # Verify 3 buttons visible on right side of title bar
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add minimize, maximize, close buttons to title bar"
```

---

### Task 9: Title bar font (#17)

Change title bar font to use the UI sans-serif font instead of the default (monospace-looking) font.

**Files:**
- Modify: `src/ecs/title_bar_system.h`

**Step 1: Set font on title bar label**

The title bar div uses `.with_label(title)` which inherits the default afterhours font. If the UI font is loaded, we should ensure the title bar uses it. Check if afterhours `ComponentConfig` has a `.with_font()` method. If not, the fix may be to ensure `theme::UI_FONT` is set as the afterhours default font during initialization in `preload.cpp`.

Check `preload.cpp` for font initialization and ensure the global afterhours font is set to the UI font.

**Step 2: Build and verify**

```bash
make all && make run   # Title bar text should look sans-serif
```

**Step 3: Commit**

```bash
git add -A && git commit -m "use UI sans-serif font for title bar text"
```

---

### Task 10: Title bar unsaved indicator (#27)

Make the unsaved-changes indicator more visible than just an asterisk.

**Files:**
- Modify: `src/ecs/title_bar_system.h`

**Step 1: Change asterisk to more visible indicator**

```cpp
// BEFORE
if (doc.isDirty) {
    title += " *";
}
// AFTER — use bullet or explicit text
if (doc.isDirty) {
    title += " [Modified]";
}
```

**Step 2: Build and verify**

```bash
make all && make run   # Edit text, verify "[Modified]" appears in title
```

**Step 3: Commit**

```bash
git add -A && git commit -m "change unsaved indicator from asterisk to [Modified] for visibility"
```

---

## Phase 3: Toolbar Improvements

### Task 11: Toolbar icons (#1)

Replace single-letter labels with descriptive text or simple Unicode glyphs until proper bitmap icons are created. This is the highest-impact change.

**Files:**
- Modify: `src/ecs/toolbar_system.h`

**Step 1: Replace labels with short descriptive text or Unicode glyphs**

Since we don't have bitmap icons yet, use recognizable Unicode symbols as a step up from single letters:

```cpp
// Standard toolbar buttons — replace .with_label("X") calls:
// File operations
.with_label("📄")   // New   (was "N") — or use "New" if Unicode doesn't render
.with_label("📂")   // Open  (was "O")
.with_label("💾")   // Save  (was "S")
.with_label("🖨")   // Print (was "P")

// Clipboard
.with_label("✂")    // Cut   (was "X")
.with_label("📋")   // Copy  (was "C")
.with_label("📌")   // Paste (was "V")

// Undo/Redo
.with_label("↩")    // Undo  (was "<")
.with_label("↪")    // Redo  (was ">")
```

**IMPORTANT:** Test if the loaded font supports these Unicode code points. If not, fall back to short text labels:
```cpp
.with_label("New")
.with_label("Opn")
.with_label("Sav")
.with_label("Prt")
.with_label("Cut")
.with_label("Cpy")
.with_label("Pst")
.with_label("Udo")
.with_label("Rdo")
```

If using text labels, increase button width to fit (change `buttonSize` to accommodate text, or use `ComponentSize{pixels(36), pixels(buttonSize)}`).

For proper pixel-art icons, create a follow-up task to draw 16x16 bitmaps and render them via afterhours texture support. Document this in `docs/plans/toolbar-icons-bitmap.md`.

**Step 2: Build and run**

```bash
make all && make run   # Verify icons/labels are readable and buttons look correct
```

**Step 3: Commit**

```bash
git add -A && git commit -m "replace single-letter toolbar labels with descriptive text/Unicode glyphs"
```

---

### Task 12: Toolbar separators — etched style (#15)

Current separators are a single dark line. Win95 uses a dark+light pair (etched look).

**Files:**
- Modify: `src/ecs/toolbar_system.h`

**Step 1: Update `absSeparator()` helper**

```cpp
// BEFORE: single dark line
inline ComponentConfig absSeparator(float x, float y, float height) {
    return ComponentConfig{}
        .with_size(ComponentSize{pixels(2), pixels(height)})
        .with_absolute_position()
        .with_translate(x, y)
        .with_custom_background(ui_imm::win95_colors::BORDER_DARK)
        .with_roundness(0.0f);
}

// AFTER: draw two lines (dark then light) for etched look
// Since ComponentConfig can only do one background, draw two divs side by side:
inline void drawEtchedSeparator(afterhours::ui::UIContext<InputAction>& ctx,
                                 afterhours::Entity& uiRoot, int baseId,
                                 float x, float y, float height) {
    // Dark line (left)
    div(ctx, mk(uiRoot, baseId),
        ComponentConfig{}
            .with_size(ComponentSize{pixels(1), pixels(height)})
            .with_absolute_position()
            .with_translate(x, y)
            .with_custom_background(ui_imm::win95_colors::BORDER_DARK)
            .with_roundness(0.0f));
    // Light line (right)
    div(ctx, mk(uiRoot, baseId + 1),
        ComponentConfig{}
            .with_size(ComponentSize{pixels(1), pixels(height)})
            .with_absolute_position()
            .with_translate(x + 1, y)
            .with_custom_background(ui_imm::win95_colors::BORDER_LIGHT)
            .with_roundness(0.0f));
}
```

Replace all `div(ctx, mk(uiRoot, btnId++), absSeparator(...))` calls with `drawEtchedSeparator(ctx, uiRoot, btnId, ...)` and increment `btnId += 2`.

**Step 2: Build and run**

```bash
make all && make run   # Separators should look etched (dark|light pair)
```

**Step 3: Commit**

```bash
git add -A && git commit -m "use etched dark+light separator pairs in toolbar"
```

---

### Task 13: Formatting button active state (#7)

Improve visual distinction when B/I/U or alignment buttons are active.

**Files:**
- Modify: `src/ecs/toolbar_system.h` (the `absToolbarButton` helper)

**Step 1: Enhance pressed/active button appearance**

The current `absToolbarButton` with `pressed=true` uses `TOOLBAR_PRESSED_BG` (160,160,160) and `BevelStyle::Sunken`. Make the distinction stronger:

```cpp
inline ComponentConfig absToolbarButton(float x, float y, float size, bool enabled = true, bool pressed = false) {
    // More distinct pressed state: darker background + sunken bevel
    afterhours::Color bg = pressed
        ? afterhours::Color{140, 140, 140, 255}  // Darker than before (was 160)
        : rlToAh(theme::BUTTON_FACE);             // Normal: 192
    afterhours::Color textColor = enabled ? rlToAh(theme::BUTTON_TEXT) : rlToAh(theme::MENU_DISABLED);

    auto config = ComponentConfig{}
        .with_size(ComponentSize{pixels(size), pixels(size)})
        .with_absolute_position()
        .with_translate(x, y)
        .with_roundness(0.0f)
        .with_custom_background(bg)
        .with_custom_text_color(textColor)
        .with_bevel(pressed ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 2.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center);

    if (!enabled) {
        config.disabled = true;
    }
    return config;
}
```

**Step 2: Build and test**

```bash
make all && make run   # Toggle Bold, verify button looks clearly pressed/sunken
```

**Step 3: Commit**

```bash
git add -A && git commit -m "darken active formatting buttons for clearer pressed state"
```

---

### Task 14: Tooltips on toolbar buttons (#4)

**Files:**
- Modify: `src/ecs/toolbar_system.h`

**Step 1: Check afterhours tooltip support**

Look for `.with_tooltip()` or similar on `ComponentConfig`. If afterhours doesn't support tooltips natively, document this as an afterhours feature request in `docs/plans/afterhours-feature-requests.md`.

If tooltips ARE supported:
```cpp
absToolbarButton(curX, btnY, buttonSize, true)
    .with_label("N")
    .with_tooltip("New (Ctrl+N)")
    .with_debug_name("btn_new")
```

If tooltips are NOT supported, use the status bar as a workaround: on hover, display the tooltip text in the status bar left field. This requires tracking which button is hovered and passing it to `StatusBarSystem`. Add a `std::string hoverTooltip` field to `ToolbarComponent`.

**Step 2: Build and test**

```bash
make all && make run   # Hover over toolbar buttons, check for tooltip/status bar text
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add tooltips to toolbar buttons"
```

---

## Phase 4: Document Area & Scroll Bar

### Task 15: Text area sunken border (#14)

**Files:**
- Modify: `src/ecs/render_system.h` (document area drawing)

**Step 1: Add sunken border around text area**

Find where the document text area background is drawn (the white rectangle). After drawing the background, draw a sunken border:

```cpp
// After drawing text area background:
// Draw sunken border (Win95 field border style)
// Dark on top/left, light on bottom/right
float x = layout.textArea.x;
float y = layout.textArea.y;
float w = layout.textArea.width;
float h = layout.textArea.height;

// Outer dark edge (top, left)
afterhours::draw_line({x, y}, {x + w, y}, {128, 128, 128, 255}); // top
afterhours::draw_line({x, y}, {x, y + h}, {128, 128, 128, 255}); // left
// Outer light edge (bottom, right)
afterhours::draw_line({x, y + h}, {x + w, y + h}, {255, 255, 255, 255}); // bottom
afterhours::draw_line({x + w, y}, {x + w, y + h}, {255, 255, 255, 255}); // right
// Inner (1px inset) for double-border effect
afterhours::draw_line({x+1, y+1}, {x+w-1, y+1}, {64, 64, 64, 255}); // top inner
afterhours::draw_line({x+1, y+1}, {x+1, y+h-1}, {64, 64, 64, 255}); // left inner
afterhours::draw_line({x+1, y+h-1}, {x+w-1, y+h-1}, {223, 223, 223, 255}); // bottom inner
afterhours::draw_line({x+w-1, y+1}, {x+w-1, y+h-1}, {223, 223, 223, 255}); // right inner
```

**Step 2: Build and verify**

```bash
make all && make run   # Document area should have visible sunken border
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add sunken bevel border around document text area"
```

---

### Task 16: Vertical scroll bar (#2)

**Files:**
- Modify: `src/ecs/render_system.h` (add scroll bar rendering)
- Modify: `src/ecs/components.h` (add scroll bar state if needed)
- Modify: `src/ecs/input_system.h` (handle scroll bar clicks)

**Step 1: Add scroll bar rendering after document area**

After rendering the document text, draw a vertical scroll bar on the right edge:

```cpp
// Scroll bar dimensions
float scrollBarWidth = theme::layout::scale(16);
float scrollBarX = layout.textArea.x + layout.textArea.width;
float scrollBarY = layout.textArea.y;
float scrollBarH = layout.textArea.height;

// Background track
afterhours::draw_rectangle({scrollBarX, scrollBarY, scrollBarWidth, scrollBarH},
                           {192, 192, 192, 255}); // BUTTON_FACE gray

// Calculate thumb position and size
int totalLines = static_cast<int>(doc.buffer.stats().lines);
int visibleLines = scroll.visibleLines;
float thumbRatio = (totalLines > 0) ? std::min(1.0f, static_cast<float>(visibleLines) / totalLines) : 1.0f;
float thumbHeight = std::max(theme::layout::scale(20), scrollBarH * thumbRatio);
float scrollRange = scrollBarH - thumbHeight;
float thumbY = scrollBarY;
if (totalLines > visibleLines && totalLines > 0) {
    thumbY += scrollRange * (static_cast<float>(scroll.offset) / (totalLines - visibleLines));
}

// Thumb (raised button)
raylib::Rectangle thumbRect = {scrollBarX + 1, thumbY, scrollBarWidth - 2, thumbHeight};
afterhours::draw_rectangle(thumbRect, {192, 192, 192, 255});
// 3D borders on thumb
afterhours::draw_line({thumbRect.x, thumbRect.y}, {thumbRect.x + thumbRect.width, thumbRect.y}, {255, 255, 255, 255});
afterhours::draw_line({thumbRect.x, thumbRect.y}, {thumbRect.x, thumbRect.y + thumbRect.height}, {255, 255, 255, 255});
afterhours::draw_line({thumbRect.x, thumbRect.y + thumbRect.height}, {thumbRect.x + thumbRect.width, thumbRect.y + thumbRect.height}, {128, 128, 128, 255});
afterhours::draw_line({thumbRect.x + thumbRect.width, thumbRect.y}, {thumbRect.x + thumbRect.width, thumbRect.y + thumbRect.height}, {128, 128, 128, 255});

// Sunken border around entire scroll bar
afterhours::draw_line({scrollBarX, scrollBarY}, {scrollBarX, scrollBarY + scrollBarH}, {128, 128, 128, 255});
afterhours::draw_line({scrollBarX, scrollBarY}, {scrollBarX + scrollBarWidth, scrollBarY}, {128, 128, 128, 255});
```

**Step 2: Adjust text area width to make room for scroll bar**

In `LayoutComponent` setup or wherever `textArea.width` is computed, subtract the scroll bar width:

```cpp
layout.textArea.width -= theme::layout::scale(16); // Make room for scroll bar
```

**Step 3: Build and run**

```bash
make all && make run   # Scroll bar visible on right edge, thumb moves when scrolling
```

**Step 4: Commit**

```bash
git add -A && git commit -m "add vertical scroll bar to document area"
```

---

## Phase 5: Status Bar Fixes

### Task 17: Status bar inconsistency (#16)

Ensure the full Word 6.0 status bar layout always shows (not just green "Auto-saved" text in some views).

**Files:**
- Modify: `src/ecs/status_bar_system.h`

**Step 1: Verify StatusBarSystem is always registered**

The `StatusBarSystem` in `status_bar_system.h` already draws the full layout. The inconsistency comes from screenshots taken before the migration. Verify in `main.cpp` that `StatusBarSystem` is always registered (not conditionally). It should already be the case.

If there's an old code path drawing a simpler status bar in `render_system.h`, find and remove it.

**Step 2: Build and verify across test modes**

```bash
make all && make e2e   # Check screenshots — all should show full status bar
```

**Step 3: Commit (if changes needed)**

```bash
git add -A && git commit -m "ensure full status bar layout is always shown"
```

---

### Task 18: Status bar abbreviation tooltips (#21)

**Files:**
- Modify: `src/ecs/status_bar_system.h`

**Step 1: Add tooltip or hover text for status indicators**

Since afterhours may not support tooltips on divs, the simplest workaround is to add a hover-based status message. When the mouse is over the right section of the status bar, display a tooltip div above it:

Check if mouse is within the right status bar area, and if so, display an overlay with explanations:
```
REC = Macro Recording | MRK = Revision Marking | EXT = Extend Selection | OVR = Overtype Mode
```

If afterhours tooltips are available, use those. Otherwise, track in `docs/plans/afterhours-feature-requests.md`.

**Step 2: Build and verify**

```bash
make all && make run   # Hover over REC/MRK area, check for explanation
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add hover explanations for status bar indicators"
```

---

## Phase 6: Selection & Visual Polish

### Task 19: Selection highlight contrast (#18)

**Files:**
- Modify: `src/ecs/render_system.h` (text rendering with selection)

**Step 1: Ensure selected text is drawn in white**

Find where text is drawn with selection highlight. The selection background is navy (0,0,128) — text on top must be white (255,255,255). Check `renderTextBuffer()` in `render_system.h`:

```cpp
// When drawing text in selection range, use SELECTION_TEXT color
if (isSelected) {
    textColor = theme::SELECTION_TEXT; // White (255,255,255)
}
```

Verify `theme::SELECTION_TEXT` is defined as white in `theme.h` — it is (line 20-21).

**Step 2: Build, type text, select it, verify it's readable**

```bash
make all && make run
```

**Step 3: Commit**

```bash
git add -A && git commit -m "ensure selected text renders in white on navy highlight"
```

---

### Task 20: Consistent spacing (#22)

**Files:**
- Modify: `src/ui/theme.h` (layout constants)

**Step 1: Audit and normalize spacing constants**

Check current spacing between UI bands:
```cpp
// In theme.h:
constexpr int TITLE_BAR_HEIGHT = 24;
constexpr int MENU_BAR_HEIGHT = 20;
constexpr int TOOLBAR_HEIGHT = 28;
constexpr int FORMATTING_BAR_HEIGHT = 28;
constexpr int RULER_HEIGHT = 20;
constexpr int STATUS_BAR_HEIGHT = 20;
```

These are fine individually. The inconsistency is in padding/gaps between them. Verify that each system positions its elements with 0 gap (butting together) — that's the Win95 convention. Check each system's Y position calculation:

```
Title bar:      Y = 0
Menu bar:       Y = TITLE_BAR_HEIGHT
Toolbar:        Y = TITLE_BAR_HEIGHT + MENU_BAR_HEIGHT
Formatting bar: Y = above + TOOLBAR_HEIGHT
Ruler:          Y = above + FORMATTING_BAR_HEIGHT
Document:       Y = above + RULER_HEIGHT
Status bar:     Y = screenHeight - STATUS_BAR_HEIGHT
```

Verify this chain is consistent across `toolbar_system.h`, `menu_ui_system.h`, `render_system.h`, and `status_bar_system.h`. Fix any gaps.

**Step 2: Build and verify**

```bash
make all && make run   # UI bands should butt together with no gaps
```

**Step 3: Commit**

```bash
git add -A && git commit -m "normalize spacing between UI chrome bands to zero gap"
```

---

### Task 21: Type hierarchy (#33)

**Files:**
- Modify: `src/ui/theme.h`

**Step 1: Define distinct font sizes for different UI elements**

```cpp
// Add to theme.h layout namespace:
constexpr int TITLE_FONT_SIZE = 14;      // Title bar — slightly smaller than body
constexpr int MENU_FONT_SIZE = 14;       // Menu bar and menu items
constexpr int TOOLBAR_FONT_SIZE = 10;    // Toolbar button labels (small)
constexpr int STATUS_FONT_SIZE = 12;     // Status bar text
constexpr int RULER_FONT_SIZE = 8;       // Ruler numbers (already used)
// FONT_SIZE (18) remains the document body default
```

**Step 2: Use these constants in each system**

Update `title_bar_system.h`, `menu_ui_system.h`, `toolbar_system.h`, `status_bar_system.h` to use the appropriate font size constant instead of hardcoded values or the global `FONT_SIZE`.

**Step 3: Build and verify**

```bash
make all && make run   # Each UI band should have appropriate text size
```

**Step 4: Commit**

```bash
git add -A && git commit -m "define type hierarchy with distinct font sizes per UI element"
```

---

## Phase 7: Menu Enhancements

### Task 22: Access keys / mnemonics (#6)

Add underlined access key letters to menu titles. This requires afterhours to support rendering underlined characters in labels.

**Files:**
- Modify: `src/ui/menu_setup.h` (add access key metadata)
- Modify: `src/ecs/menu_ui_system.h` (render underlines)

**Step 1: Add access key data to Menu/MenuItem**

```cpp
// In win95_widgets.h, add to Menu and MenuItem:
struct MenuItem {
    // ... existing fields ...
    int accessKeyIndex = -1;  // Index of underlined character (-1 = none)
};

struct Menu {
    // ... existing fields ...
    int accessKeyIndex = -1;  // Index of underlined character in label
};
```

**Step 2: Set access keys in `menu_setup.h`**

```cpp
fileMenu.accessKeyIndex = 0;     // F in File
editMenu.accessKeyIndex = 0;     // E in Edit
viewMenu.accessKeyIndex = 0;     // V in View
insertMenu.accessKeyIndex = 0;   // I in Insert
formatMenu.accessKeyIndex = 1;   // o in Format (F taken by File)
toolsMenu.accessKeyIndex = 0;    // T in Tools
tableMenu.accessKeyIndex = 2;    // b in Table (T taken by Tools)
helpMenu.accessKeyIndex = 0;     // H in Help
```

**Step 3: Render underlines in MenuUISystem**

In `menu_ui_system.h`, when drawing menu header labels, draw the label normally then draw a 1px underline under the access key character. This requires knowing the character's X position — use `theme::MeasureUIText()` to measure the substring before the access key character.

If afterhours doesn't support per-character underline rendering, document this as a feature request and skip the visual underline for now. The access key data is still useful for keyboard handling later.

**Step 4: Build and verify**

```bash
make all && make run   # Menu titles should show underlined access key letter
```

**Step 5: Commit**

```bash
git add -A && git commit -m "add access key underlines to menu titles"
```

---

### Task 23: Context menus (#24)

**Files:**
- Modify: `src/ecs/input_system.h` (detect right-click)
- Modify: `src/ecs/menu_ui_system.h` (render context menu)
- Modify: `src/ecs/components.h` (add context menu state)

**Step 1: Add context menu state to MenuComponent**

```cpp
// In components.h MenuComponent:
bool contextMenuOpen = false;
float contextMenuX = 0;
float contextMenuY = 0;
std::vector<win95::MenuItem> contextMenuItems;
```

**Step 2: Detect right-click in input system**

In `input_system.h`, detect right mouse button press in the document area and populate context menu items (Cut, Copy, Paste, separator, Select All):

```cpp
if (raylib::IsMouseButtonPressed(raylib::MOUSE_BUTTON_RIGHT)) {
    menu.contextMenuOpen = true;
    menu.contextMenuX = raylib::GetMouseX();
    menu.contextMenuY = raylib::GetMouseY();
    menu.contextMenuItems = {
        {"Cut", "Ctrl+X", doc.buffer.hasSelection(), false, nullptr},
        {"Copy", "Ctrl+C", doc.buffer.hasSelection(), false, nullptr},
        {"Paste", "Ctrl+V", true, false, nullptr},
        {"", "", false, true, nullptr},
        {"Select All", "Ctrl+A", true, false, nullptr}
    };
}
```

**Step 3: Render context menu in MenuUISystem**

Draw a dropdown at `(contextMenuX, contextMenuY)` using the same styling as regular dropdown menus. Close on click outside or item selection.

**Step 4: Build and test**

```bash
make all && make run   # Right-click in document area should show context menu
```

**Step 5: Commit**

```bash
git add -A && git commit -m "add right-click context menu in document area"
```

---

## Phase 8: Ruler & Document Polish

### Task 24: Ruler alignment (#28)

**Files:**
- Modify: `src/ecs/render_system.h` (ruler drawing code)

**Step 1: Fix ruler tick mark calculation**

The ruler code in `render_system.h` (around line 630-682) uses `pixelsPerInch = scaleInt(72)`. The issue is integer rounding causing inconsistent spacing at different scales. Use floating-point math:

```cpp
float pixelsPerInch = theme::layout::scale(72.0f);
float rulerStartX = theme::layout::scale(50.0f);
int maxInches = static_cast<int>((layout.screenWidth - rulerStartX) / pixelsPerInch);

for (int inch = 0; inch <= maxInches; ++inch) {
    float x = rulerStartX + inch * pixelsPerInch;
    // Use float x for all positioning, cast to int only for final draw calls
    int ix = static_cast<int>(x);
    // ... draw marks at ix
}
```

**Step 2: Build and verify**

```bash
make all && make run   # Ruler marks should be evenly spaced
```

**Step 3: Commit**

```bash
git add -A && git commit -m "fix ruler tick marks to use float math for consistent spacing"
```

---

### Task 25: Ruler margin handles (#29)

**Files:**
- Modify: `src/ecs/render_system.h` (ruler drawing)
- Modify: `src/ecs/input_system.h` (handle drag on ruler)
- Modify: `src/ecs/components.h` (add margin state)

**Step 1: Draw margin indicators on ruler**

Draw small triangles at the left and right margin positions on the ruler:

```cpp
// Left margin handle (downward triangle)
float leftMarginX = rulerStartX + layout.leftMargin * pixelsPerInch;
// Draw filled triangle pointing down
afterhours::draw_triangle(
    {leftMarginX - 4, rulerY + 2},
    {leftMarginX + 4, rulerY + 2},
    {leftMarginX, rulerY + 10},
    {0, 0, 0, 255});

// Right margin handle
float rightMarginX = rulerStartX + layout.rightMargin * pixelsPerInch;
// Similar triangle
```

**Step 2: Add drag handling for margin handles**

In `input_system.h`, detect mouse press on the ruler area near a margin handle, and drag to adjust margin values in `LayoutComponent`.

**Step 3: Build and test**

```bash
make all && make run   # Drag ruler handles to adjust margins
```

**Step 4: Commit**

```bash
git add -A && git commit -m "add draggable margin handles to ruler"
```

---

## Phase 9: Menu Dropdown Visual Polish

### Task 26: Drop shadows on menus (#34)

**Files:**
- Modify: `src/ecs/menu_ui_system.h`

**Step 1: Add shadow behind dropdown menus**

Before drawing the dropdown container, draw a slightly offset dark rectangle behind it:

```cpp
// Shadow (4px offset, semi-transparent black)
div(ctx, mk(entity, shadowId),
    ComponentConfig{}
        .with_size(/* same as dropdown */)
        .with_absolute_position()
        .with_translate(dropdownX + 4, dropdownY + 4)
        .with_custom_background(afterhours::Color{0, 0, 0, 80})
        .with_roundness(0.0f)
        .with_render_layer(9));  // Behind the dropdown (layer 10)

// Then draw the dropdown at layer 10 as before
```

**Step 2: Build and verify**

```bash
make all && make run   # Open a menu, verify shadow appears behind dropdown
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add drop shadow behind menu dropdowns"
```

---

## Phase 10: Responsive & Motion

### Task 27: Responsive layout (#35)

**Files:**
- Modify: `src/ecs/toolbar_system.h`
- Modify: `src/ecs/menu_ui_system.h`

**Step 1: Handle narrow window widths**

In `ToolbarRenderSystem`, check if `screenWidth` is below a threshold and skip the formatting toolbar or use a compact layout:

```cpp
bool compactMode = screenWidth < 600;

if (compactMode) {
    // Hide formatting toolbar, show only standard toolbar
    // Or reduce button padding
}
```

In `MenuUISystem`, ensure dropdown menus don't clip the right edge of the screen:

```cpp
// Clamp dropdown X so it stays on screen
float maxDropdownX = screenWidth - dropdownWidth;
float clampedX = std::min(menuX, maxDropdownX);
```

**Step 2: Build and test at small window size**

```bash
make all && make run   # Resize window small, verify no clipping/overlap
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add responsive layout for narrow window widths"
```

---

### Task 28: Motion/animation (#36)

Document as afterhours feature request — afterhours immediate-mode UI rebuilds every frame, so CSS-style transitions aren't directly applicable. Simple workarounds:

**Files:**
- Create: `docs/plans/afterhours-feature-requests.md` (if not exists)

**Step 1: Document the need**

Add to `docs/plans/afterhours-feature-requests.md`:
```markdown
## Animation / Transition Support
- Menu dropdowns should slide/fade open (100-200ms)
- Toast notifications should slide in from top and fade out
- Button press should have brief visual feedback (50ms darker flash)
- Afterhours would need a transition system that interpolates component properties between frames
```

**Step 2: Commit**

```bash
git add -A && git commit -m "document animation feature request for afterhours"
```

---

### Task 29: Progress indicators (#38)

**Files:**
- Modify: `src/ecs/render_system.h` (wrap long operations)
- Modify: `src/ui/ui_context.h` (add progress toast helper)

**Step 1: Add progress feedback for save/export operations**

For operations that could take time (PDF export, HTML export, RTF export), show a brief status message:

```cpp
// Before export:
toast_notify::info("[...] Exporting PDF...", 1.0f);
// After export completes, show success or error toast (already exists)
```

This is a minimal approach. A proper progress bar would require afterhours support (document in feature requests).

**Step 2: Build and verify**

```bash
make all && make run   # Trigger export, verify progress toast appears briefly
```

**Step 3: Commit**

```bash
git add -A && git commit -m "add progress feedback toast for export operations"
```

---

## Phase 11: Afterhours-Dependent Items (Deferred)

These items require afterhours library changes or investigation. Document workarounds and feature requests.

### Task 30: Hover state on buttons (#20)

**Needs:** Afterhours `ComponentConfig` needs a `.with_hover_background()` or hover state callback.

**Current state:** Buttons show raised/sunken borders but no hover highlight.

**Workaround:** Track mouse position manually and change button background when hovering. This is expensive in immediate mode (checking bounds for every button every frame).

**Document in:** `docs/plans/afterhours-feature-requests.md`

```markdown
## Hover State for Buttons
- Need: `.with_hover_background(color)` on ComponentConfig
- Or: `button()` return value includes hover state
- Win95 style: lighten background to (220,220,220) on hover
- Current workaround: manual mouse position check per button (not implemented, too costly)
```

---

### Task 31: Focus indicators (#12)

**Needs:** Afterhours keyboard focus system with visible focus rectangles.

**Current state:** Afterhours has `HandleTabbing` and `ComputeVisualFocusId` systems registered, but no visible focus ring rendering.

**Workaround:** Check if afterhours renders focus indicators in `RenderImm`. If not, document.

**Document in:** `docs/plans/afterhours-feature-requests.md`

```markdown
## Visible Focus Indicators
- Need: Dotted rectangle border on focused UI element (Win95 style)
- Afterhours has `ComputeVisualFocusId` — does `RenderImm` draw a focus ring?
- If not, need `.with_focus_border(style, color)` on ComponentConfig
- Or post-render overlay that draws focus rect based on `visual_focus_id`
```

---

### Task 32: Dropdown arrow glyph (#8) — Maybe

**Needs:** Afterhours dropdown to render a proper ▼ glyph instead of appending " v" to the label.

**Current state:** We manually append " v" to dropdown labels in `toolbar_system.h`.

**Workaround:** Replace " v" with " ▼" if the font supports it:
```cpp
std::string styleLabel = toolbar.currentStyle + " ▼";
```

**Document in:** `docs/plans/afterhours-feature-requests.md`

```markdown
## Dropdown Arrow Glyph
- Dropdowns currently show " v" as arrow indicator
- Need: afterhours dropdown component to auto-render a proper ▼ triangle
- Workaround: Use Unicode ▼ if font supports it
- Better: Draw a filled triangle glyph programmatically next to dropdown text
```

---

### Task 33: Commit afterhours feature requests doc

```bash
git add -A && git commit -m "document afterhours feature requests for hover, focus, dropdown glyph, animation"
```

---

## Summary

| Phase | Tasks | Estimated Time |
|-------|-------|---------------|
| 1. Quick Wins (data/text) | 1-7 | 45 min |
| 2. Title Bar | 8-10 | 20 min |
| 3. Toolbar Improvements | 11-14 | 40 min |
| 4. Document Area & Scroll | 15-16 | 30 min |
| 5. Status Bar Fixes | 17-18 | 15 min |
| 6. Selection & Polish | 19-21 | 20 min |
| 7. Menu Enhancements | 22-23 | 40 min |
| 8. Ruler & Doc Polish | 24-25 | 25 min |
| 9. Menu Visual Polish | 26 | 10 min |
| 10. Responsive & Motion | 27-29 | 20 min |
| 11. Afterhours Deferred | 30-33 | 15 min (docs only) |
| **Total** | **33 tasks** | **~4.5 hours** |

Afterhours-dependent items are documented in `docs/plans/afterhours-feature-requests.md` rather than modifying the submodule. When you're back, review that doc and coordinate upstream changes.
