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
1. Use `vendor/afterhours` library (and dependencies) for UI/app.
2. Prefer no library changes early; if needed, log in `AfterhoursGaps.md` with app-side workaround.
3. P0: E2E/Jest/integration + unit testing early, including screenshot-based UI verification and manual visual checks.
4. Copy from `wm_afterhours`: `vendor/`, `.clang-format`, `.gitignore`, `makefile`, `.cursor` rules, select `src` preload/window init items, and fonts from `resources/`.
5. Style: Windows 95 base with Mac OS 3.1 accents.
6. Branching: app changes on `main`; any `vendor/afterhours` changes on `wordproc` branch only.

## Success Criteria
1. [ ] P0 testing stack in place: unit + integration + e2e/screenshot tests with manual visual verification.
2. [ ] App launches and supports typing, caret movement, save, and open.
3. [ ] Basic formatting (bold/italic) and font selection implemented.
4. [ ] `style_guide.md` created with Win95/Mac3.1 design, animation, and interaction rules.
5. [ ] `AfterhoursGaps.md` exists and documents any required library changes with app-side workarounds.
6. [ ] Required assets/config copied from `wm_afterhours` and integrated (vendor, makefile, .clang-format, .gitignore, .cursor rules, preload/window init, fonts).

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

### 8) Testing & Tooling (P0)
- [ ] Choose and set up unit test framework (Catch2 or GTest).
- [ ] Set up integration + E2E tests early (Jest/automation or equivalent).
- [ ] Add screenshot-based UI verification (automated capture during interactions).
- [ ] Add a visible/manual test flow to "use your eyes" to confirm behavior.
- [ ] Document how to run tests and capture screenshots.

### 9) AfterhoursGaps
- [ ] Create `AfterhoursGaps.md` and log any needed library changes.
- [ ] Provide app-side workaround for each gap while avoiding vendor changes.

---

## Ralph Instructions
1. Work on the next incomplete criterion (marked [ ])
2. Check off completed criteria (change [ ] to [x])
3. Run tests after changes
4. Commit your changes frequently
5. When ALL criteria are [x], output: `<ralph>COMPLETE</ralph>`
6. If stuck on the same issue 3+ times, output: `<ralph>GUTTER</ralph>`
