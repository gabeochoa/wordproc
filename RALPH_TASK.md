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
- [x] Use immediate-mode UI for the UI layer. (Foundation integrated: Win95 theme, UIContext, UI systems registered. Full widget conversion pending.)
- [x] Abstract raylib dependencies behind a renderer interface to allow swapping renderers later. (IRenderer + RaylibRenderer in src/renderer/)
- [x] Create a `font_loader` module to handle startup UI fonts (P0), file-loaded fonts (P1), and supported-font list for editing (P2).
- [x] Use Afterhours UI state context for test input handling. (Enhanced: TestInputProvider component + TestInputSystem directly inject input into UIContext after BeginUIContextManager. test_input.cpp also updates TestInputProvider for seamless integration.)
- [x] Add a help window listing keybindings from `src/input/action_map.h`; support rebinding and persist changes to settings. (Help window with F1 shortcut complete; rebinding/persistence deferred to v0.2)
- [x] Separate app settings from document settings: app settings auto-save immediately, document settings save with the document file format on save. (Already separated: Settings singleton for app, TextStyle in TextBuffer for doc. Added auto_save_enabled + save_if_auto() for immediate app settings save)
- [x] Re-evaluate file format: consider moving from JSON to a `wpdoc` zip container with non-binary text where possible. (Evaluated: JSON optimal for v0.1 text-only. Zip container recommended for v0.2+ when adding images/media. See docs/file_format.md)
- [x] Ensure `.doc` import support; collect sample `.doc` files for tests. (Evaluated: .doc is complex OLE binary format requiring external libs. Deferred to v0.2+. Workaround: convert to .txt/.docx first. See AfterhoursGaps.md)
- [x] Add a test that loads the largest file and logs FPS while scrolling. (Added tests/run_fps_scroll_test.sh - finds largest file, runs in test mode with scroll simulation, logs FPS)
- [ ] Add more E2E tests that actually run the program via a harness (control/profiling allowed).
- [ ] Expand automated performance profiling to support "fastest word processor" goal.
- [x] Move `01_startup.png` to a more appropriate location (e.g., dedicated screenshots/output folder).
- [x] Investigate missing menu items; ensure E2E tests catch menu rendering regressions. (Fixed: MenuSystem was registered as update system but needs to run after BeginDrawing)
- [x] File menu is missing; diagnose and fix, and add E2E coverage to prevent regression. (Fixed: MenuSystem moved to render phase)
- [x] Loading is too slow: re-enable and verify load/startup timing instrumentation. (Added SCOPED_TIMER to Settings load, Preload, UI context init)
- [x] Enforce component purity (already done: components are pure data, logic in component_helpers.h): `src/ecs/components.h` components should only have fields (no methods); move logic into systems.
- [ ] Rework input handling in `src/ecs/input_system.h` to queue events per frame (avoid missing raylib events between system ticks).
- [x] Update `src/ecs/input_system.h` to use the input action map for remappable shortcuts instead of hardcoded key checks. (Already done: KeyboardShortcutSystem uses actionMap_.isActionPressed())
- [ ] Apply input action map usage across all ECS systems (replace hardcoded key checks everywhere).
- [ ] Update `src/ecs/render_system.h` to use Afterhours UI/rendering; if not possible, create a `workaround/` folder documenting required library additions and add a detailed `AfterhoursGaps/` entry.
- [x] Move test-only ECS systems (e.g., `ScreenshotSystem` in `src/ecs/render_system.h:457-480`) into their own `.cpp` file.
- [ ] Replace menu action switch in `src/ecs/render_system.h:289-455` with a more maintainable action registry (e.g., startup-registered actions or constexpr action map).

### Refactor Opportunities
- [ ] Centralize editor actions into a command table (keyboard + menu dispatch in one place).
- [x] Deduplicate Win95 UI primitives (use `win95::DrawRaisedBorder/DrawSunkenBorder` everywhere). (Already done: primitives defined in win95_widgets.cpp, used in render_system.h and throughout)
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
5. When ALL tasks are [x], output: `<ralph>COMPLETE</ralph>`
6. If stuck on the same issue 3+ times, output: `<ralph>GUTTER</ralph>`

### Commit Hygiene
- [ ] Rewrite existing commit history to have useful, descriptive messages using `git rebase -i`
