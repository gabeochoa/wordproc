---
task: Build a Word Processor (Afterhours)
test_command: "make test"
---

# Task: Word Processor (Afterhours)

## Context (verbatim)
- "Update ralph task with these instructions."
- "look at ~/p/wm_afterhours copy over the main outline of the project."
- "we are going to use vendor/afterhours library (and its associated dependencies) to write our own word processor."
- "feel free to start with small features first (just typing saving a file, opening a file. bold italic whatever)"
- "then add new font selection and whatever else you think."
- "i want it to be styled like a windows 95 program please. or maybe liked macos 3.1"
- "add all of these to ralph making tasks and subtasks for each of these items."
- "include a way to unit test the code. and if you need feel free to pull in catch or gtest for testing."
- "make sure to start by unittesting/ jest testing e2e testing early."
- "for the main repo. you can make all your changes on main. but for vendor/afterhours do all of your changes on the wordproc branch as we dont have permission to make changes without review to the underlying library."
- "try your best to avoid making changes to the library at first. if there are any changes you need then write up a AfterhoursGaps.md file with what you need and add a workaround in your own folder for now."
- "Make sure when testing you have some way of opening the program and screenshotting the UI as you do things . this is why i suggested jest."
- "you should copy over the vendor folder for sure as well as clang-format, makefile, .cursor rules ."
- "include in your list of tasks a style_guide.md on what kinds of design we want as well as how things should be animated and interactions should work."
- "from src/ we want to copy over some of the preload and other raylib and window initialization items since it will make our life easier (another place for feedback around AfterhoursGaps.md)"
- "We also likely want to grab some of the fonts from resources/"
- "also bring over the gitignore"
- "include all of the context ive given you in the ralph task so that it can have it"
- "make sure that E2E / jest / integration testing is p0 because we need that working so you can figure out if things are working as expected. you will need to use your eyes"

## Project Summary
Build a word processor using the vendored Afterhours library and dependencies. Style it like Windows 95 with Mac OS 3.1 accents. Start with small features first, then expand.

## Requirements
1. Use `vendor/afterhours` library (and dependencies) broadly (UI, input, window, animation, etc.) so we can surface gaps in `AfterhoursGaps.md`.
2. Proactively identify new Afterhours features and provide feedback on current items as we build the app.
2. Prefer no library changes early; if needed, log in `AfterhoursGaps.md` with app-side workaround.
3. P0: E2E/Jest/integration + unit testing early, including screenshot-based UI verification and manual visual checks.
4. Copy from `wm_afterhours`: `vendor/`, `.clang-format`, `.gitignore`, `makefile`, `.cursor` rules, select `src` preload/window init items, and fonts from `resources/`.
5. Style: Windows 95 base with Mac OS 3.1 accents.
6. Branching: app changes on `main`; any `vendor/afterhours` changes on `wordproc` branch only.
7. Prefer immutable data and pure functions where they simplify correctness or testing.
8. Support rendering/import for `.doc`, `.txt`, and `.md` files (even if the native save format is custom).
9. Track startup time from `./wordproc file.txt` to fully rendered + interactive, targeting <= 100ms cold start.
10. Document format version is always `v0.1` for now.
11. Cursor overlay must align precisely with the actual edit position (no rightward offset).
12. Executable name is `wordproc` (not `ui_tester`).

## Success Criteria
1. [x] P0 testing stack in place: unit + integration + e2e/screenshot tests with manual visual verification.
2. [x] App launches and supports typing, caret movement, save, and open.
3. [x] Basic formatting (bold/italic) and font selection implemented.
4. [x] `style_guide.md` created with Win95/Mac3.1 design, animation, and interaction rules.
5. [x] `AfterhoursGaps.md` exists and documents any required library changes with app-side workarounds.
6. [x] Required assets/config copied from `wm_afterhours` and integrated (vendor, makefile, .clang-format, .gitignore, .cursor rules, preload/window init, fonts).
7. [x] Core text storage and rendering path uses a data-oriented SoA layout with measurable performance wins.
8. [x] `./wordproc file.txt` to fully interactive <= 100ms cold start, measured and tracked.
9. [x] Document view supports scrolling (keyboard + mouse wheel/trackpad).
10. [x] Menu buttons trigger real actions (no-op buttons removed or wired).

## Task Breakdown (use folder-outline from `wm_afterhours`)

### 1) Vendor & Build System
- [x] Copy `vendor/` from `wm_afterhours` (keep any `vendor/afterhours` modifications on `wordproc` branch only).
- [x] Copy `.clang-format`, `.gitignore`, `makefile`, and `.cursor` rules.
- [x] Verify build succeeds with copied vendor + build config.

### 2) Resources
- [x] Copy selected fonts from `wm_afterhours/resources/fonts/` (document licenses if needed).
- [x] Decide which additional resources (images, cursors) are needed for the UI skin.

### 3) Core Runtime (preload / window / raylib)
- [x] Copy preload and window initialization pieces from `wm_afterhours/src/` as a starting point.
- [x] Document any missing needs in `AfterhoursGaps.md` and implement temporary workarounds in app code.

### 4) Editor Engine (small features first)
- [x] Basic text buffer and caret model.
- [x] Keyboard input for typing, backspace/delete, and line breaks.
- [x] Selection model (start/end, shift-select) as needed.
- [x] Simple layout for lines and wrapping.

### 5) File I/O
- [x] Save document to file.
- [x] Open document from file.
- [x] Persist formatting metadata (at least bold/italic and font choice).

### 6) Formatting & Fonts
- [x] Bold + italic toggles.
- [x] Font selection UI (start with a small set).
- [x] Font size adjustments and defaults.

### 7) UI/UX & Styling (Win95 base + Mac OS 3.1 accents)
- [x] Implement classic window chrome (title bar, borders, menu strip).
- [x] Buttons, menus, dialogs styled for Win95/Mac3.1 hybrid.
- [x] Cursor, selection, and focus states visually clear.
- [x] Create `style_guide.md` defining colors, typography, spacing, animations, and interaction states.
- [x] Add document scrolling (mouse wheel/trackpad + scrollbar).
- [x] Wire menu buttons to real actions (disable or remove placeholders).

### 8) Testing & Tooling (P0)
- [x] Choose and set up unit test framework (Catch2 or GTest).
- [x] Set up integration + E2E tests early (Jest/automation or equivalent).
- [x] Add screenshot-based UI verification (automated capture during interactions).
- [x] Add a visible/manual test flow to "use your eyes" to confirm behavior.
- [x] Document how to run tests and capture screenshots.
- [x] Add load-time regression suite that opens all `test_files/public_domain/*.txt` and writes a timing report (cold start + ready-to-interact).
- [x] Define report format (CSV preferred), include filename, size, cold-start time, ready-to-interact time, and pass/fail vs 100ms target; keep it chart-friendly.
- [x] Define cold-start procedure (fresh process, no warm caches, first-frame interactive signal).
- [x] Store a baseline report and diff against it to flag regressions.

### 9) Performance & Data-Oriented Architecture (SoA)
- [x] Replace `TextBuffer` AoS (`std::vector<std::string>`) with SoA storage (contiguous char store + line offsets/lengths or piece table).
- [x] Reduce per-insert allocations by using a gap buffer or piece table per document/line.
- [x] Update layout to avoid copying substrings (store spans/offsets instead of `std::string` per wrapped line).
- [x] Add benchmarks for insert, delete, and layout operations (document sizes + typing bursts).
- [x] Ensure rendering uses cached glyph/layout data to avoid per-frame re-layout.
- [x] Instrument and log startup time from CLI launch to interactive; add perf budget checks (<= 100ms cold start).

### 10) Code Review Follow-ups
- [x] Implement text rendering (draw buffer content + wrapped lines).
- [x] Draw caret and selection highlight with blink/animation timing.
- [x] Handle selection deletion on typing/backspace/delete.
- [x] Add word/line navigation (Ctrl+Arrow, Home/End, PageUp/PageDown).
- [x] Add clipboard integration (copy/cut/paste).
- [x] Add undo/redo with command history.
- [x] Ensure save/open path handles formatting metadata (basic rich text format or JSON).
- [x] Add window title + dirty-state indicator on edits.
- [x] Fix caret positioning to use per-glyph advance/metrics (not max-width); add a regression case like "llllll".
- [x] Define document file format/extension (e.g., .wpdoc) and versioned schema (fixed at `v0.1` for now) with backward-compat to plaintext and importers for .txt/.md/.doc.
- [x] Add load/save error reporting (surface parse errors and fallback behavior).
- [x] Decide on per-range styles vs global style state and update model accordingly. (Decision: Global style for v0.1; per-range deferred to v0.2)
- [x] Build a format validator and invalid-fixture generator to load malformed files, verifying error messaging or warning-banner fallback render.
- [x] Define validator rules (required fields, types, size limits, supported versions).
- [x] Populate `test_files/should_fail/` with malformed JSON, truncated files, wrong versions, and oversized payloads.
- [x] Populate `test_files/should_pass/` with edge-case but valid files (empty, huge, mixed encoding, markdown input).

### 11) AfterhoursGaps
- [x] Create `AfterhoursGaps.md` and log any needed library changes.
- [x] Provide app-side workaround for each gap while avoiding vendor changes.
- [x] Review Afterhours APIs used and add feedback/new feature ideas as they emerge.

## Feedback Tasks
- [x] Replace `std::snprintf` with `fmt::` or `std::format` for status text formatting in `src/main.cpp`.
- [x] Move hardcoded colors (e.g., around `src/main.cpp:793`) into a Theme file to centralize styling.
- [ ] Refactor the main loop in `src/main.cpp:323-833` to use ECS system functionality instead of a single file.
- [ ] Use immediate-mode UI for the UI layer.
- [x] Move input handling to an enum-based action map so keys can be remapped later.
- [ ] Abstract raylib dependencies behind a renderer interface to allow swapping renderers later.
- [x] Stop using Gaegu-Bold for default UI elements; choose a more readable small-font UI face. (UI uses raylib default font; Gaegu is only for document text on user selection)
- [x] Move Win95 menu setup (`src/main.cpp:249-303`) into its own file.
- [x] Replace `std::printf` at `src/main.cpp:207-214` with the project's logging system.
- [x] Add a timing header around `src/main.cpp:188-199` for easier profiling without full setup.
- [x] Move the utility definitions in `src/main.cpp:15-165` (colors, config structs, drawing helpers, etc.) into separate files.
- [ ] Create a `font_loader` module to handle startup UI fonts (P0), file-loaded fonts (P1), and supported-font list for editing (P2), refactoring `src/preload.cpp:84-220`.
- [x] Review `src/settings.cpp:1-217` for improved setup; consider zpp-bits serialization (from pharmasea) if it simplifies settings I/O. (Simplified by removing unused volume/post-processing; zpp-bits deferred)
- [x] Remove volume-related APIs from `src/settings.h:33-41`.
- [x] Remove post-processing APIs from `src/settings.h:44-46`.
- [x] Review `src/settings.h:1-48` and add any missing settings needed for the app.
- [x] Replace custom Win95 widgets in `src/ui/win95_widgets.h:1-85` with vendor/afterhours UI library usage; document gaps in `AfterhoursGaps.md`. (Gap documented; Afterhours lacks themeable widget library - keeping custom widgets as workaround)
- [ ] Use Afterhours UI state context for test input handling in `src/testing/test_input.h:1-55`.
- [x] Review `src/testing/test_input.cpp:5-13` macro undefines and decide whether to keep/replace them. (Kept: necessary to avoid recursion in raylib-mocked test input)
- [ ] Add a help window listing keybindings from `src/input/action_map.h`; support rebinding and persist changes to settings.
- [x] If `src/engine/input_injector.cpp:1-173` is test-only, move it into the testing folder. (Already in src/testing/)
- [x] Evaluate using immutable structures for text layout in `src/editor/text_layout.h:1-107`. (Already uses SoA LayoutResult with immutable parallel arrays; further immutability deferred)
- [x] Add tests that validate on-screen content while scrolling.
- [ ] Add keyboard shortcut presets: system default, Windows Ctrl-based, and macOS Cmd-based.
- [x] Add unsaved-changes indicator (`*`) in the UI when the document is dirty.
- [ ] Separate app settings from document settings: app settings auto-save immediately, document settings save with the document file format on save.
- [ ] Re-evaluate file format: consider moving from JSON to a `wpdoc` zip container with non-binary text where possible.
- [ ] Ensure `.doc` import support; collect sample `.doc` files from https://file-examples.com/index.php/sample-documents-download/sample-doc-download/ for tests.

---

## Ralph Instructions
1. Work on the next incomplete criterion (marked [ ])
2. Check off completed criteria (change [ ] to [x])
3. Run tests after changes
4. Commit your changes frequently
5. When ALL criteria are [x], output: `<ralph>COMPLETE</ralph>`
6. If stuck on the same issue 3+ times, output: `<ralph>GUTTER</ralph>`

---

## Refactor Opportunities (reduce LOC / simplify)
- Centralize editor actions into a command table (keyboard + menu dispatch in one place).
- Deduplicate Win95 UI primitives (use `win95::DrawRaisedBorder/DrawSunkenBorder` everywhere).
- Pick a single text layout path (remove legacy or SoA layout to avoid parallel APIs).
- Remove or wire `RenderCache` (avoid unused code paths).
- Factor repeated line-span offset shifts in `TextBuffer` edits into a helper.
- Make font loading table-driven instead of manual per-font wiring.
- Run clang-format using the rules from `/Users/gabeochoa/p/pharmasea/.clang-format`.
