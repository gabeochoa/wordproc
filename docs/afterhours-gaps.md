# Open Afterhours integration gaps

Checked September 14, 2026, against `c5cfd359967c6bb165fd40b6a2bc14d5009adc2f`. This list contains unresolved upstream limitations. A working Wordproc workaround does not mean the shared API has been fixed. Adoption tasks and their checks live in [todo.md](../todo.md).

## Platform and file services

- **Web stack reporting.** `src/crash_handler.h:19` includes native `execinfo.h` under Emscripten. Upstream needs a platform-specific stack reporter. Wordproc currently supplies a web-only compatibility header using `emscripten_get_callstack`.
- **Web backend separation.** `src/backends/sokol/backend.h:63` declares Metal device/capture functions, and the shared backend references them in WebGL builds. Wordproc supplies unsupported native hooks and browser canvas resizing. Upstream must select backend registration and capture functions by platform.
- **Headless run lifecycle.** `RunConfig::display` is not honored by Sokol's `run()` at `src/backends/sokol/backend.h:847`; it enters `sapp_run`. Wordproc uses `graphics::init(Config{Headless})`, bounded frame callbacks, and explicit shutdown. Upstream should provide that lifecycle through `run()`.
- **Nonblocking native dialogs.** Queueing requests does not make the backend asynchronous: `src/plugins/native_dialogs_macos.mm:48` calls `runModal`. Wordproc pumps requests outside ECS work and keeps its frame reentrancy guard. A shared asynchronous completion path would remove that restriction.
- **Atomic-write concurrency and durability.** `src/plugins/files.h:121` uses a fixed `.afh-tmp` sibling; the writer does not fsync the file or directory. Wordproc uses one writer per target. Unique temporary files and an explicit durable-write option are still upstream work. A streaming atomic writer would also let large RTF exports preserve bounded memory instead of building a complete output string.

## Input and UI

- **Injected right-button pressed edges.** `src/plugins/e2e_testing/test_input.h:259` handles left and middle presses but returns false for an injected right press. Wordproc detects the rising right-button hold. Upstream should expose consistent pressed/held/released behavior for all supported buttons and test right-click dismissal and reopening.
- **Exact modifier matching.** Explicit chords at `src/plugins/input_system.h:1017` accept extra modifiers. Duplicate suppression at `:1071` does not disambiguate two explicit chords such as Ctrl+Z and Ctrl+Shift+Z. Wordproc currently requires exact Ctrl/Shift/Alt matching. Add an explicit matching policy or deterministic specificity rule before replacing that behavior.
- **Horizontal dropdown placement.** `src/plugins/ui/imm_components.h:1978` applies the overlay's vertical correction but translates X by zero. Apply both placement coordinates and test dropdowns at the right edge, at increased scale, and inside offset parents.
- **Menu row styling.** `src/plugins/ui/component_config.h:1260` inherits explicit custom colors but not theme-usage colors, hover styling, or padding. Rows in `src/plugins/ui/menu.h:182` therefore cannot fully follow a parent menu configuration. Wordproc supplies an explicit background. Provide row-level styling or a documented inheritance contract, and verify text padding alongside shortcut gutters.
- **Long and marked menus.** `src/plugins/ui/menu.h:20` has no checked/radio/mixed item state. `menu_list` sizes its panel from all rows without bounded scrolling. Add item marks and a viewport-aware scroll limit before Wordproc's long Format menu can use it without losing behavior. Preserve keyboard access to the final item at small window sizes.

## Test infrastructure

- **Sokol draw capture.** Draw assertion commands are available, but Sokol drawing helpers do not call `capture::record`; the none and raylib implementations do. Add equivalent hooks in `src/backends/sokol/drawing_helpers.h`, including text, primitives, layer, and entity attribution. Verify captured calls before adopting `expect_drawn` for Metal. Keep pixel checks because recorded commands do not prove GPU output.
- **Shared harness argument compatibility.** `src/plugins/e2e_testing/harness.h:54` accepts separated `--test-script path` arguments but not Wordproc's `--test-script=path` form or its `--e2e-timeout` spelling. Normalize supported forms before replacing the app parser, with tests for missing values and invalid timeouts.

All source paths in this file are relative to `vendor/afterhours`. The vendored source is unchanged. Resolved Wordproc integration details belong in [upgrade verification](upgrade-verification.md), not this open-gap list.
