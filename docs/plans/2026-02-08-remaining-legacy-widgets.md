# Remaining Legacy Widgets Migration

## Current State

After the menu bar, status bar, and ruler, the remaining legacy `win95::` widgets in `win95_widgets.cpp` (862 lines) are:

### Still Used

1. **`DrawRaisedBorder` / `DrawSunkenBorder`** (~50 lines): Called from `util::drawRaisedBorder` / `util::drawSunkenBorder` in render_system.h for the title bar, menu bar background, text area, and status bar borders. Also used internally by other win95 widgets.

2. **`DrawButton`** (~50 lines): Used by `DrawMessageDialog` and `DrawInputDialog`. NOT used directly in any system — only as a building block for dialogs.

3. **`DrawCheckbox`** (~45 lines): Used by `DrawMessageDialog`. Not called elsewhere.

4. **`DrawMessageDialog`** (~65 lines): Legacy modal dialog with OK/Cancel buttons. **Status**: Already partially replaced by `afterhours::modal::info()` in `MenuUISystem`. Still used in `render_system.h` for some dialogs (Find/Replace, Page Setup).

5. **`DrawInputDialog`** (~90 lines): Legacy text input modal. **Status**: Already replaced by `ah_modal_input.h` (`InputDialogState` + `input_dialog`). Check if any code still calls it.

6. **`DrawToolbarButton`** (~50 lines): Legacy toolbar button with icon drawing. **Status**: Toolbar was migrated to afterhours in `toolbar_system.h`. Check if still called.

7. **`DrawToolbarSeparator`** (~10 lines): Vertical line between toolbar groups. **Status**: Check if still called after toolbar migration.

8. **`DrawDropdownButton`** / **`DrawDropdownList`** (~110 lines): Combo box widgets for style/font selection. Used by toolbar dropdowns. **Status**: Toolbar was migrated but may still use these for the font/style dropdowns.

9. **`DrawToolbarIcon`** (~200 lines): Draws programmatic icons (new, open, save, print, cut, copy, paste, undo, redo, B/I/U, L/C/R/J alignment). Large switch statement of raw raylib draw calls.

### Title Bar

10. **Title bar rendering** (lines 622-638 in `render_system.h`): Draws the blue title bar with "Wordproc - Untitled *". ~16 lines of raw raylib.

### Text Area & Document Rendering

11. **Text area background** (lines 716-732): Draws the document area background (white or gray for paged mode).
12. **`renderTextBuffer`** (lines 81-539 of `render_system.h`): The core text rendering function — 450+ lines of glyph-by-glyph drawing with selection highlighting, caret rendering, line numbers, etc. This is NOT a widget — it's the document renderer. It should remain as raw drawing code; afterhours UI is not suited for glyph-level text rendering.

## What Needs to Happen

### Phase 1: Delete Dead Code
Audit which `win95::` functions are still called after toolbar migration. Likely candidates for deletion:
- `DrawToolbarButton` — if toolbar fully migrated
- `DrawToolbarSeparator` — if toolbar fully migrated  
- `DrawToolbarIcon` — if toolbar fully migrated (200 lines!)

### Phase 2: Migrate Borders
Replace `DrawRaisedBorder`/`DrawSunkenBorder` usage with afterhours `BevelStyle`. The afterhours theme system already supports `BevelStyle::Raised` and `BevelStyle::Sunken`. This can be done incrementally as each container (title bar, text area, etc.) gets migrated.

### Phase 3: Delete Legacy Dialogs
Once all callers of `DrawMessageDialog` and `DrawInputDialog` use afterhours modals, delete both functions along with `DrawButton` and `DrawCheckbox` (which are only used by dialogs).

### Phase 4: Title Bar
Move title bar rendering into an afterhours `div` with the blue background and text label. Simple migration.

## Does It Need Afterhours Changes?

**For Phase 1-3: No.** These are just deletions and caller updates.

**For toolbar icons (DrawToolbarIcon):** If the toolbar migration still uses `DrawToolbarIcon` internally, we need an afterhours way to draw custom icons. Options:
- Use font-based icons (icon font)
- Use texture atlas
- Use a `HasCustomDraw` component (same idea as ruler)
- Keep `DrawToolbarIcon` as a utility function called from within the afterhours toolbar rendering — it's just draw calls, not a widget.

## How to Validate

1. **Build and test**: After each deletion, run `make all` to verify no linker errors.
2. **Run full e2e suite**: All 18 `pass_*` tests must pass.
3. **Screenshot review**: Verify no visual regressions — borders still look correct, dialogs still render, toolbar icons still appear.
4. **grep for win95::**: Track the count of `win95::` references going to zero.

## Open Questions

1. **Is `DrawToolbarIcon` still called?** The toolbar was migrated to `toolbar_system.h` using afterhours, but does it still call the legacy icon drawing function?
2. **Which dialogs still use `DrawMessageDialog`?** Need to audit all callers — Find/Replace, Page Setup, Save As, etc.
3. **`util::drawRaisedBorder` / `util::drawSunkenBorder`**: Are these wrappers around `win95::` or separate implementations? If separate, deleting `win95::` versions may be sufficient.
4. **Text area background**: Should the text area container become an afterhours `div`? It's borderline — the text area is just a rectangle with a sunken border, and the document rendering inside is always raw. A hybrid approach (afterhours container, raw content) is likely best.
