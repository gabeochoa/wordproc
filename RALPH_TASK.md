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
- [ ] Use immediate-mode UI for the UI layer.
- [ ] Abstract raylib dependencies behind a renderer interface to allow swapping renderers later.
- [ ] Create a `font_loader` module to handle startup UI fonts (P0), file-loaded fonts (P1), and supported-font list for editing (P2).
- [ ] Use Afterhours UI state context for test input handling.
- [ ] Add a help window listing keybindings from `src/input/action_map.h`; support rebinding and persist changes to settings.
- [ ] Separate app settings from document settings: app settings auto-save immediately, document settings save with the document file format on save.
- [ ] Re-evaluate file format: consider moving from JSON to a `wpdoc` zip container with non-binary text where possible.
- [ ] Ensure `.doc` import support; collect sample `.doc` files for tests.
- [ ] Add a test that loads the largest file and logs FPS while scrolling.
- [ ] Add more E2E tests that actually run the program via a harness (control/profiling allowed).
- [ ] Expand automated performance profiling to support "fastest word processor" goal.
- [x] Move `01_startup.png` to a more appropriate location (e.g., dedicated screenshots/output folder).
- [ ] Investigate missing menu items; ensure E2E tests catch menu rendering regressions.
- [ ] File menu is missing; diagnose and fix, and add E2E coverage to prevent regression.
- [ ] Loading is too slow: re-enable and verify load/startup timing instrumentation.
- [ ] Enforce component purity: `src/ecs/components.h` components should only have fields (no methods); move logic into systems.
- [ ] Rework input handling in `src/ecs/input_system.h` to queue events per frame (avoid missing raylib events between system ticks).
- [ ] Update `src/ecs/input_system.h` to use the input action map for remappable shortcuts instead of hardcoded key checks.
- [ ] Apply input action map usage across all ECS systems (replace hardcoded key checks everywhere).
- [ ] Update `src/ecs/render_system.h` to use Afterhours UI/rendering; if not possible, create a `workaround/` folder documenting required library additions and add a detailed `AfterhoursGaps/` entry.
- [ ] Move test-only ECS systems (e.g., `ScreenshotSystem` in `src/ecs/render_system.h:457-480`) into their own `.cpp` file.
- [ ] Replace menu action switch in `src/ecs/render_system.h:289-455` with a more maintainable action registry (e.g., startup-registered actions or constexpr action map).

### Refactor Opportunities
- [ ] Centralize editor actions into a command table (keyboard + menu dispatch in one place).
- [ ] Deduplicate Win95 UI primitives (use `win95::DrawRaisedBorder/DrawSunkenBorder` everywhere).
- [ ] Pick a single text layout path (remove legacy or SoA layout to avoid parallel APIs).
- [ ] Remove or wire `RenderCache` (avoid unused code paths).
- [ ] Factor repeated line-span offset shifts in `TextBuffer` edits into a helper.
- [ ] Make font loading table-driven instead of manual per-font wiring.
- [ ] Run clang-format using the rules from `/Users/gabeochoa/p/pharmasea/.clang-format`.

---

## Ralph Instructions
1. Work on the next incomplete task (marked [ ])
2. Check off completed tasks (change [ ] to [x])
3. Run tests after changes
4. Commit your changes frequently with **descriptive commit messages** (e.g., "Add E2E test for menu rendering", not "update" or "wip")
5. **Go back and rewrite existing commit history** to have useful, descriptive messages using `git rebase -i`
6. When ALL tasks are [x], output: `<ralph>COMPLETE</ralph>`
7. If stuck on the same issue 3+ times, output: `<ralph>GUTTER</ralph>`
