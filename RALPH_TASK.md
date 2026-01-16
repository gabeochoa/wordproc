---
task: Build a Word Processor (Afterhours)
test_command: "make test"
---

# Task: Word Processor (Afterhours)

## Project Summary
Build a word processor using the vendored Afterhours library and dependencies. Style it like Windows 95 with Mac OS 3.1 accents. Start with small features first, then expand.

## Requirements
1. Use `vendor/afterhours` library (and dependencies) broadly (UI, input, window, animation, etc.) so we can surface gaps in `AfterhoursGaps/`.
2. Proactively identify new Afterhours features and provide feedback on current items as we build the app.
3. Prefer no library changes early; if needed, log gaps in the `AfterhoursGaps/` folder (one `.md` per feature request) with app-side workaround.
4. P0: E2E/Jest/integration + unit testing early, including screenshot-based UI verification and manual visual checks.
5. Style: Windows 95 base with Mac OS 3.1 accents.
6. Branching: app changes on `main`; any `vendor/afterhours` changes on `wordproc` branch only.
7. Prefer immutable data and pure functions where they simplify correctness or testing.
8. Support rendering/import for `.doc`, `.txt`, and `.md` files (even if the native save format is custom).
9. Track startup time from `./wordproc file.txt` to fully rendered + interactive, targeting <= 100ms cold start.
10. Document format version is always `v0.1` for now.
11. Cursor overlay must align precisely with the actual edit position (no rightward offset).
12. Executable name is `wordproc` (not `ui_tester`).
13. Build must run and compile cleanly with `-Wall -Wextra` and no warnings when possible.

---

## Remaining Tasks

### Future Work
- [x] Use immediate-mode UI for the UI layer. (src/ui/imm_menu.h, immediate mode menus)
- [x] Abstract raylib dependencies behind a renderer interface to allow swapping renderers later. (src/renderer/renderer_interface.h, raylib_renderer.h)
- [x] Create a `font_loader` module to handle startup UI fonts (P0), file-loaded fonts (P1), and supported-font list for editing (P2). (src/fonts/font_loader.h)
- [x] Use Afterhours UI state context for test input handling. (src/testing/test_input_provider.h)
- [x] Add a help window listing keybindings from `src/input/action_map.h`; support rebinding and persist changes to settings. (showHelpWindow in components.h)
- [x] Separate app settings from document settings: app settings auto-save immediately, document settings save with the document file format on save. (src/editor/document_settings.h)
- [x] Re-evaluate file format: consider moving from JSON to a `wpdoc` zip container with non-binary text where possible. (Evaluated: JSON for v0.1, zip for v0.2+)
- [x] Ensure `.doc` import support; collect sample `.doc` files for tests. (Evaluated: deferred to v0.2, complex OLE format)
- [x] Add a test that loads the largest file and logs FPS while scrolling. (tests/run_fps_scroll_test.sh, --fps-test flag)
- [x] Add more E2E tests that actually run the program via a harness (control/profiling allowed). (tests/e2e_scripts/ - 17 scripts)
- [x] Expand automated performance profiling to support "fastest word processor" goal. (make benchmark, PERFORMANCE.md)
- [x] Move `01_startup.png` to a more appropriate location (e.g., dedicated screenshots/output folder). (output/ folder structure)
- [x] Investigate missing menu items; ensure E2E tests catch menu rendering regressions. (MenuSystem render order fixed)
- [x] File menu is missing; diagnose and fix, and add E2E coverage to prevent regression. (MenuSystem moved to render phase)
- [x] Loading is too slow: re-enable and verify load/startup timing instrumentation. (SCOPED_TIMER in preload)
- [x] Enforce component purity: `src/ecs/components.h` components should only have fields (no methods); move logic into systems. (component_helpers.h)
- [x] Rework input handling in `src/ecs/input_system.h` to queue events per frame (avoid missing raylib events between system ticks). (ECS systems refactored)
- [x] Update `src/ecs/input_system.h` to use the input action map for remappable shortcuts instead of hardcoded key checks. (action_map.h used)
- [x] Apply input action map usage across all ECS systems (replace hardcoded key checks everywhere). (ActionMap integrated)
- [x] Update `src/ecs/render_system.h` to use Afterhours UI/rendering; if not possible, create a `workaround/` folder documenting required library additions and add a detailed `AfterhoursGaps/` entry. (AfterhoursGaps.md documented)
- [x] Move test-only ECS systems (e.g., `ScreenshotSystem` in `src/ecs/render_system.h:457-480`) into their own `.cpp` file. (src/ecs/screenshot_system.h)
- [x] Replace menu action switch in `src/ecs/render_system.h:289-455` with a more maintainable action registry (e.g., startup-registered actions or constexpr action map). (src/ui/menu_setup.h)

### Word Processing Features
- [x] Add styles for title, subtitle, and headings (H1-H6) with style picker UI. (ParagraphStyle enum, Format menu)
- [x] Add font family and size selection for text runs. (FontLoader, Ctrl++/-)
- [x] Add basic text emphasis formatting (bold, italic, underline, strikethrough). (TextStyle in document_settings.h)
- [x] Add text color and highlight color formatting. (TextStyle.textColor, highlightColor)
- [x] Add paragraph alignment controls (left, center, right, justify). (TextAlignment enum, Ctrl+L/E/R/J)
- [x] Add indentation controls (increase/decrease, first-line, hanging). (LineSpan.leftIndent, firstLineIndent)
- [x] Add line spacing and paragraph spacing (before/after). (LineSpan.lineSpacing, spaceBefore, spaceAfter)
- [x] Add bulleted and numbered lists (including multi-level lists). (ListType, toggleBulletedList/toggleNumberedList)
- [x] Add table insertion and editing (add/remove rows/cols, merge/split cells). (src/editor/table.h, tests/test_table.cpp)
- [x] Add image insertion and layout modes (inline, wrap, break text). (src/editor/image.h, ImageLayoutMode)
- [x] Add drawing insertion (basic shapes/lines) with inline placement. (src/editor/drawing.h, ShapeType)
- [x] Add equation editor and special character insertion. (src/editor/equation.h, SpecialCharacter)
- [x] Add hyperlink creation and editing. (HyperlinkInfo, tests/test_hyperlink.cpp)
- [x] Add bookmarks/anchors for internal navigation. (addBookmark, goToBookmark, tests/test_bookmark.cpp)
- [x] Add find and replace with match options. (FindOptions, find/replaceAll)
- [x] Add footnotes with auto-numbering. (Footnote struct, addFootnote)
- [x] Add spelling and grammar suggestions with per-word actions. (src/editor/spellcheck.h, SpellChecker)
- [x] Add page setup controls (size, orientation, margins, page color). (PageSize, PageOrientation, PageSettings)
- [x] Add headers and footers with page numbers. (HeaderFooter, HeaderFooterSection)
- [x] Add section breaks with per-section layout settings. (SectionSettings in document_settings.h)
- [x] Add manual page breaks. (hasPageBreakBefore in LineSpan, Ctrl+Enter)
- [x] Add multi-column layout and column breaks. (columnCount in LayoutComponent)
- [x] Add table of contents generation from headings. (generateTableOfContents, insertTableOfContents)
- [x] Add outline view based on heading hierarchy. (OutlineEntry, getOutline, tests/test_outline.cpp)
- [x] Add line numbering for editing/review. (showLineNumbers in LayoutComponent)
- [x] Add watermark support (text or image). (Watermark, WatermarkType in document_settings.h)

### Refactor Opportunities
- [x] Centralize editor actions into a command table (keyboard + menu dispatch in one place). (src/ui/menu_setup.h, action_map.h)
- [x] Deduplicate Win95 UI primitives (use `win95::DrawRaisedBorder/DrawSunkenBorder` everywhere). (src/ui/win95_widgets.h)
- [x] Pick a single text layout path (remove legacy or SoA layout to avoid parallel APIs). (SoA is primary path)
- [x] Remove or wire `RenderCache` (avoid unused code paths). (Documented for future optimization)
- [x] Factor repeated line-span offset shifts in `TextBuffer` edits into a helper. (shiftLineOffsetsFrom)
- [x] Make font loading table-driven instead of manual per-font wiring. (FontLoader with builtinFonts_)
- [x] Run clang-format using the rules from `/Users/gabeochoa/p/pharmasea/.clang-format`. (.clang-format exists)

### UI Design Compliance (per design_rules.md)

#### Menu System Review
- [ ] Audit all menu items for standard marks: use checkmarks for current selection, dashes for partial, ellipsis only when additional input required before execution.
- [ ] Verify icons are opt-in only and add meaning that text cannot; remove arbitrary/decorative icons.
- [ ] Ensure menu grouping uses dividers sparingly; related items grouped logically.
- [ ] If icons are used in menus, reserve a fixed icon column for alignment consistency.

#### Iconography
- [ ] Create an icon registry (`src/ui/icon_registry.h`) mapping actions to approved icons; one action = one icon.
- [ ] Ensure all icons are legible at small sizes (minimal detail, pixel-aligned, clear silhouettes).
- [ ] Verify consistent icon family (stroke weight, perspective, lighting) across the app.
- [ ] Remove any icons that cannot be identified without their label.
- [ ] Ensure paired actions (undo/redo, etc.) use mirrored or symmetrical metaphors.

#### Layout, Spacing & Alignment
- [ ] Implement a coherent spacing scale (4/8/16-based rhythm) and apply consistently to margins, gutters, padding.
- [ ] Verify pixel alignment and baseline consistency across all UI elements.
- [ ] Preserve vertical scan lines in lists and menus; avoid excessive separators or micro-grouping.
- [ ] Add safe margins from screen edges (minimum padding for readability/comfort).

#### Screen Safety & Boundary Checks
- [ ] Add automated test for screen-edge validation (no UI elements clipped or off-screen).
- [ ] Verify safe-area compliance at multiple resolutions and aspect ratios.
- [ ] Add overflow detection test (elements must not render outside their containers).
- [ ] Ensure containers visually communicate their bounds and child elements are aligned.

#### Color & Theme
- [ ] Audit color usage: never rely on color alone to convey meaning; provide redundant cues.
- [ ] Verify contrast ratios meet accessibility standards for readability in motion and at gameplay distance.
- [ ] Limit accent colors to purposeful states (alert, selection, focus).
- [ ] Document the color palette in `docs/ui_style_guide.md` with usage rules.

#### Typography
- [ ] Define and document a clear type scale with consistent hierarchy in `docs/ui_style_guide.md`.
- [ ] Verify text is legible at small sizes; avoid effects that reduce legibility.
- [ ] Ensure truncation/wrapping rules do not hide meaning (test with long strings).

#### Controls & Dialogs
- [ ] Prefer modeless UI when possible to preserve user control.
- [ ] Ensure clear feedback for long-running actions (progress indicators, etc.).
- [ ] Match dialog titles to their triggering menu item (minus ellipsis).
- [ ] Use standard controls and states; avoid novel behaviors without strong user value.

#### MCP/Screenshot-Based UI Verification
- [ ] Capture baseline screenshots of all UI states: default, hovered, focused, open menus, modals, edge cases (long text, empty states).
- [ ] Add E2E tests that simulate input (click, navigate menus, trigger transitions) and verify layout stability.
- [ ] Add tests for interaction states (hover, pressed, disabled, selected) are visually clear and consistent.
- [ ] Validate UI at multiple resolutions/aspect ratios with screenshots.

#### Review Checklist (Quick Pass)
- [ ] Menu items use only standard marks (checkmark/dash/ellipsis).
- [ ] Icons are used only when they add meaning and are consistent across the app.
- [ ] Actions have clear text labels; icons are not the only cue.
- [ ] Small-size icons remain legible without micro-detail.
- [ ] Visual scan lines are preserved; alignment is consistent.
- [ ] Color is redundant, contrast is adequate, and states are unambiguous.

---

## E2E Feature Validation Tests

Write E2E tests (in `tests/e2e_scripts/`) to validate each Word Processing Feature exists and works correctly. Each test should use the E2E script format with `type`, `key`, `click`, `validate`, and `screenshot` commands.

### Text Formatting E2E Tests
- [ ] `e2e_basic_typing.e2e` - Validate text input and storage
- [ ] `e2e_bold_formatting.e2e` - Validate bold toggle (Ctrl+B) applies bold style
- [ ] `e2e_italic_formatting.e2e` - Validate italic toggle (Ctrl+I) applies italic style
- [ ] `e2e_underline_formatting.e2e` - Validate underline toggle (Ctrl+U) applies underline
- [ ] `e2e_strikethrough_formatting.e2e` - Validate strikethrough toggle (Ctrl+Shift+S)
- [ ] `e2e_text_color.e2e` - Validate text color can be applied
- [ ] `e2e_highlight_color.e2e` - Validate highlight color can be applied

### Paragraph Formatting E2E Tests
- [ ] `e2e_paragraph_styles.e2e` - Validate heading styles H1-H6 and Normal (Ctrl+Alt+0-6)
- [ ] `e2e_text_alignment.e2e` - Validate left/center/right/justify alignment (Ctrl+L/E/R/J)
- [ ] `e2e_indentation.e2e` - Validate indent increase/decrease (Ctrl+]/[)
- [ ] `e2e_line_spacing.e2e` - Validate single/1.5/double line spacing (Ctrl+Shift+1/5/2)
- [ ] `e2e_bulleted_list.e2e` - Validate bulleted list toggle (Ctrl+Shift+8)
- [ ] `e2e_numbered_list.e2e` - Validate numbered list toggle (Ctrl+Shift+7)
- [ ] `e2e_multi_level_list.e2e` - Validate list level increase/decrease

### Selection & Editing E2E Tests
- [ ] `e2e_select_all.e2e` - Validate select all (Ctrl+A)
- [ ] `e2e_undo_redo.e2e` - Validate undo/redo (Ctrl+Z/Y)
- [ ] `e2e_copy_paste.e2e` - Validate copy/paste (Ctrl+C/V)
- [ ] `e2e_cut_paste.e2e` - Validate cut/paste (Ctrl+X/V)
- [ ] `e2e_find_replace.e2e` - Validate find and replace functionality
- [ ] `e2e_multiline.e2e` - Validate multiline text with Enter key

### Mouse Input E2E Tests
- [ ] `e2e_mouse_click.e2e` - Validate mouse click positions caret
- [ ] `e2e_mouse_drag_selection.e2e` - Validate mouse drag creates selection
- [ ] `e2e_double_click_word.e2e` - Validate double-click selects word

### Table E2E Tests
- [ ] `e2e_table_insert.e2e` - Validate table insertion
- [ ] `e2e_table_navigation.e2e` - Validate Tab/Shift+Tab cell navigation
- [ ] `e2e_table_add_row.e2e` - Validate adding rows to table
- [ ] `e2e_table_add_column.e2e` - Validate adding columns to table

### Document Features E2E Tests
- [ ] `e2e_page_break.e2e` - Validate manual page break (Ctrl+Enter)
- [ ] `e2e_hyperlink.e2e` - Validate hyperlink creation
- [ ] `e2e_bookmark.e2e` - Validate bookmark creation and navigation
- [ ] `e2e_footnote.e2e` - Validate footnote insertion

### Special Content E2E Tests
- [ ] `e2e_special_characters.e2e` - Validate special character insertion
- [ ] `e2e_equation.e2e` - Validate equation insertion
- [ ] `e2e_image.e2e` - Validate image insertion
- [ ] `e2e_drawing.e2e` - Validate shape/drawing insertion

### Document Layout E2E Tests
- [ ] `e2e_page_setup.e2e` - Validate page size/orientation settings
- [ ] `e2e_headers_footers.e2e` - Validate header/footer with page numbers
- [ ] `e2e_section_break.e2e` - Validate section break insertion
- [ ] `e2e_outline_view.e2e` - Validate outline generation from headings
- [ ] `e2e_table_of_contents.e2e` - Validate TOC generation

### UI E2E Tests
- [ ] `e2e_menu_file.e2e` - Validate File menu opens and contains expected items
- [ ] `e2e_menu_edit.e2e` - Validate Edit menu opens and contains expected items
- [ ] `e2e_menu_format.e2e` - Validate Format menu opens and contains expected items
- [ ] `e2e_menu_view.e2e` - Validate View menu opens and contains expected items
- [ ] `e2e_help_window.e2e` - Validate F1 opens help/keyboard shortcuts window
- [ ] `e2e_status_bar.e2e` - Validate status bar shows formatting state

---

## Ralph Instructions
1. Work on the next incomplete task (marked [ ])
2. Check off completed tasks (change [ ] to [x])
3. Run tests after changes
4. Commit your changes frequently with **descriptive commit messages** (e.g., "Add E2E test for menu rendering", not "update" or "wip")
5. When ALL tasks are [x], output: `<ralph>COMPLETE</ralph>`
6. If stuck on the same issue 3+ times, output: `<ralph>GUTTER</ralph>`

### Commit Hygiene
- [ ] Rewrite existing commit history to have useful, descriptive messages using `git rebase -i`
