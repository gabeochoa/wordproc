# Zig and Afterhours upgrade verification

The build migration was verified against the original Afterhours pin before updating the dependency.

- Native Zig build passed with Apple Clang.
- All 160 baseline unit tests passed, with 1,685 assertions.
- A repeated native build cached every compile and link step.
- Three baseline native UI scripts passed. Four Find-dialog screenshots were black; those captures do not prove visual correctness.
- Baseline evidence is under `output/upgrade/baseline/`.
- Updated unit tests passed: 164 cases and 1,700 assertions, including shared clipboard scope and failed-save preservation.

Run `nice -n 10 python3 scripts/verify_upgrade.py` for the integration journeys. It copies document fixtures and isolates settings and outputs in a temporary working directory. `--native` exercises a real window; the default uses offscreen Metal. `--all` includes every positive script, and `--match` selects a subset. Reports and screenshots are written under `output/upgrade/verification/` unless `--output` is supplied.

## Adopted capabilities

Afterhours moved from `51ad3d9cc353bf07f83a180231554bec852dc6b9` to `c5cfd359967c6bb165fd40b6a2bc14d5009adc2f`. The submodule is clean.

| Capability | Wordproc integration |
| --- | --- |
| Scoped clipboard providers | Document commands and Afterhours text fields share the same clipboard; test sessions use a scoped memory provider. |
| Typed native dialog queue | Open and Save As distinguish selection, cancellation, unsupported platforms, and errors. |
| Atomic file writing | Documents and settings preserve the old file if writing fails. |
| Context menus and popup placement | Cut, Copy, Paste, and Select All use the framework menu with viewport fitting and focus restoration. |
| Widget draw callbacks | Toolbar icons render with their widgets; the separate overlay pass is gone. |
| Text-field editing | Actual key collection, Shift selection, word movement/deletion, undo/redo, and Escape dismissal are connected. |
| Modal lifecycle | Closed modals retire their backdrop and restore focus; the duplicate backdrop renderer is removed. |
| Window state | The framework receives current dimensions before UI layout and pointer handling. |
| Offscreen capture | Bounded native tests render through headless Metal and capture after the completed frame. |
| Bounded profiling | An optional build flag enables the framework collector and E2E profile commands. |

Font-atlas, text measurement, clipping, stale-submission, layout, and ECS improvements arrive through the dependency update. Wordproc retains its rich document model and native renderer. The optional filesystem watcher, binding-editor persistence, chart widgets, and profiler panel are not wired into new product features here; external-file conflict handling and customizable shortcuts need app-level behavior first. Audio and game-specific additions do not apply to this app.

## Verified behavior

- Unit tests: 164 cases, 1,700 assertions passed.
- Focused upgrade replay: 27 of 28 scripts passed. Document drag selection remains unimplemented in the app-owned document view; its test fails rather than pretending that moving the pointer selected text.
- Context-menu geometry: all four corners pass bounds checks from framework layout dumps. A real native-window replay also passes and produces non-black screenshots.
- Web: Emscripten 6.0.9 builds; Chrome with SwiftShader renders the app, accepts text, opens and dismisses Find, and resizes without runtime errors. Captures were visually inspected. Browser chords hold their keys for 100 ms so modifiers span a rendered frame.
- Profiling smoke: 155 system samples were captured. That run reported 280.7 FPS and 25.75 ms p99, in an unoptimized headless build while other work was running. This proves instrumentation, not a performance improvement or a latency guarantee. The permissive smoke thresholds only detect missing or unusable measurements.

Evidence: `output/upgrade/final/`, `output/upgrade/geometry/`, `output/upgrade/native-window/`, `output/upgrade/profile/`, and `output/upgrade/web-final/`. Contact sheets are under `output/upgrade/`.

The first broad replay hit its 240-second limit after 91 passes and 18 failures. The runner now allows 900 seconds for `--all`. The complete replay finished in 291 seconds: **160 passed, 21 failed, all 181 scripts ran**. Failures include stale menu names, unsupported validation properties, exact-string numeric expectations, and unfinished document actions. They have not all been compared against the old dependency, so this report does not classify all 21 as pre-existing. The upgrade is not an all-green regression run.

Failed scripts:

```
e2e_focus_mode             e2e_headers_footers       e2e_image
e2e_line_spacing           e2e_mouse_drag_selection e2e_multi_level_list
e2e_outline_view           e2e_page_break           e2e_page_setup
e2e_paragraph_styles       e2e_rich_report          e2e_section_break
e2e_special_characters     e2e_status_bar           e2e_table_add_column
e2e_table_add_row          e2e_table_navigation     e2e_text_color
e2e_toolbar_new_document   e2e_toolbar_undo_redo     e2e_toolbar_visual_check
```

The broad run used the profiling build before the final window-size and menu-color corrections. The focused replay and browser checks were rerun on the final default build. Its four context-menu geometry assertions pass. The broad run's empty geometry field came from an earlier subtree selector; `output/upgrade/final/verification.json` contains the corrected geometry evidence.

A repeated final native build passed all 29 steps with every compilation, link, install, and resource step cached. Logs are copied to `output/upgrade/build-cache.log` and `output/upgrade/unit.log`. The existing `vendor/fmt/core.h` change was preserved.
