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
1. [ ] P0 testing stack in place: unit + integration + e2e/screenshot tests with manual visual verification.
2. [ ] App launches and supports typing, caret movement, save, and open.
3. [ ] Basic formatting (bold/italic) and font selection implemented.
4. [ ] `style_guide.md` created with Win95/Mac3.1 design, animation, and interaction rules.
5. [ ] `AfterhoursGaps.md` exists and documents any required library changes with app-side workarounds.
6. [ ] Required assets/config copied from `wm_afterhours` and integrated (vendor, makefile, .clang-format, .gitignore, .cursor rules, preload/window init, fonts).
7. [ ] Core text storage and rendering path uses a data-oriented SoA layout with measurable performance wins.
8. [ ] `./wordproc file.txt` to fully interactive <= 100ms cold start, measured and tracked.
9. [ ] Document view supports scrolling (keyboard + mouse wheel/trackpad).
10. [ ] Menu buttons trigger real actions (no-op buttons removed or wired).

## Task Breakdown (use folder-outline from `wm_afterhours`)

### 1) Vendor & Build System
- [ ] Copy `vendor/` from `wm_afterhours` (keep any `vendor/afterhours` modifications on `wordproc` branch only).
- [ ] Copy `.clang-format`, `.gitignore`, `makefile`, and `.cursor` rules.
- [ ] Verify build succeeds with copied vendor + build config.

### 2) Resources
- [ ] Copy selected fonts from `wm_afterhours/resources/fonts/` (document licenses if needed).
- [ ] Decide which additional resources (images, cursors) are needed for the UI skin.

### 3) Core Runtime (preload / window / raylib)
- [ ] Copy preload and window initialization pieces from `wm_afterhours/src/` as a starting point.
- [ ] Document any missing needs in `AfterhoursGaps.md` and implement temporary workarounds in app code.

### 4) Editor Engine (small features first)
- [ ] Basic text buffer and caret model.
- [ ] Keyboard input for typing, backspace/delete, and line breaks.
- [ ] Selection model (start/end, shift-select) as needed.
- [ ] Simple layout for lines and wrapping.

### 5) File I/O
- [ ] Save document to file.
- [ ] Open document from file.
- [ ] Persist formatting metadata (at least bold/italic and font choice).

### 6) Formatting & Fonts
- [ ] Bold + italic toggles.
- [ ] Font selection UI (start with a small set).
- [ ] Font size adjustments and defaults.

### 7) UI/UX & Styling (Win95 base + Mac OS 3.1 accents)
- [ ] Implement classic window chrome (title bar, borders, menu strip).
- [ ] Buttons, menus, dialogs styled for Win95/Mac3.1 hybrid.
- [ ] Cursor, selection, and focus states visually clear.
- [ ] Create `style_guide.md` defining colors, typography, spacing, animations, and interaction states.
- [ ] Add document scrolling (mouse wheel/trackpad + scrollbar).
- [ ] Wire menu buttons to real actions (disable or remove placeholders).

### 8) Testing & Tooling (P0)
- [ ] Choose and set up unit test framework (Catch2 or GTest).
- [ ] Set up integration + E2E tests early (Jest/automation or equivalent).
- [ ] Add screenshot-based UI verification (automated capture during interactions).
- [ ] Add a visible/manual test flow to "use your eyes" to confirm behavior.
- [ ] Document how to run tests and capture screenshots.
- [ ] Add load-time regression suite that opens all `test_files/public_domain/*.txt` and writes a timing report (cold start + ready-to-interact).
- [ ] Define report format (CSV preferred), include filename, size, cold-start time, ready-to-interact time, and pass/fail vs 100ms target; keep it chart-friendly.
- [ ] Define cold-start procedure (fresh process, no warm caches, first-frame interactive signal).
- [ ] Store a baseline report and diff against it to flag regressions.

### 9) Performance & Data-Oriented Architecture (SoA)
- [ ] Replace `TextBuffer` AoS (`std::vector<std::string>`) with SoA storage (contiguous char store + line offsets/lengths or piece table).
- [ ] Reduce per-insert allocations by using a gap buffer or piece table per document/line.
- [ ] Update layout to avoid copying substrings (store spans/offsets instead of `std::string` per wrapped line).
- [ ] Add benchmarks for insert, delete, and layout operations (document sizes + typing bursts).
- [ ] Ensure rendering uses cached glyph/layout data to avoid per-frame re-layout.
- [ ] Instrument and log startup time from CLI launch to interactive; add perf budget checks (<= 100ms cold start).

### 10) Code Review Follow-ups
- [ ] Implement text rendering (draw buffer content + wrapped lines).
- [ ] Draw caret and selection highlight with blink/animation timing.
- [ ] Handle selection deletion on typing/backspace/delete.
- [ ] Add word/line navigation (Ctrl+Arrow, Home/End, PageUp/PageDown).
- [ ] Add clipboard integration (copy/cut/paste).
- [ ] Add undo/redo with command history.
- [ ] Ensure save/open path handles formatting metadata (basic rich text format or JSON).
- [ ] Add window title + dirty-state indicator on edits.
- [ ] Fix caret positioning to use per-glyph advance/metrics (not max-width); add a regression case like "llllll".
- [ ] Define document file format/extension (e.g., .wpdoc) and versioned schema (fixed at `v0.1` for now) with backward-compat to plaintext and importers for .txt/.md/.doc.
- [ ] Add load/save error reporting (surface parse errors and fallback behavior).
- [ ] Decide on per-range styles vs global style state and update model accordingly. (Decision: Global style for v0.1; per-range deferred to v0.2)
- [ ] Build a format validator and invalid-fixture generator to load malformed files, verifying error messaging or warning-banner fallback render.
- [ ] Define validator rules (required fields, types, size limits, supported versions).
- [ ] Populate `test_files/should_fail/` with malformed JSON, truncated files, wrong versions, and oversized payloads.
- [ ] Populate `test_files/should_pass/` with edge-case but valid files (empty, huge, mixed encoding, markdown input).

### 11) AfterhoursGaps
- [ ] Create `AfterhoursGaps.md` and log any needed library changes.
- [ ] Provide app-side workaround for each gap while avoiding vendor changes.
- [ ] Review Afterhours APIs used and add feedback/new feature ideas as they emerge.

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
