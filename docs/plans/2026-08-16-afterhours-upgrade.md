# wordproc ← afterhours upgrade

Bump the vendored afterhours submodule from `51ad3d9` to `origin/main` (`17b982c`,
**235 commits**) and adopt the features wordproc had been working around.

Each `T#` below is **one commit**. Every commit must leave `make -j10 all` and
`make -j10 test` green (baseline: 160 test cases / 1685 assertions passing).

Build env on this machine: `export PKG_CONFIG_PATH="$HOME/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"`.
Judge success by compile + link + unit tests; a fresh binary that gets SIGKILLed
on *run* is Santa lockdown, not a build failure.

---

## Status

| # | Task | Feature request | State |
|---|------|-----------------|-------|
| T0 | Baseline on old pin | — | ✅ build + 160 tests green |
| T1 | Bump submodule + fix sokol impl TU | — | ✅ `8f6e671` |
| T2 | Adopt `imm::context_menu` | #15 | ✅ `24a88ca` |
| T3 | Focus ring | #5 | ✅ `413e32e` |
| T5 | Toolbar icons via `with_on_draw_fg` | #1 / #9 | ✅ `05e6fe1` |
| T6 | Dropdown triangles in-pass, overlay system deleted | #4 | ✅ `7252ec5` |
| T4 | Drop shadows on popups | #10 | ✅ `e5470bc` (not the imm menu — see below) |
| T11 | Refresh `docs/afterhours-feature-requests.md` | — | ✅ |
| T7 | Access-key underlines | #2 | ⛔ dropped — no upstream API, and wordproc never had them (the doc said it did) |
| T8 | Styled labels / font weight | #12 | ⬜ deferred — needs eyes on the result |
| T9 | Menu + toast animation | #6 | ⬜ deferred — changes E2E frame timing, needs the E2E suite |
| T10 | E2E right-click on real input | #14 | ⛔ blocked — upstream models `right_down` but nothing sets it |
| T12 | Full verification sweep | — | ✅ clean build + unit tests (`da42e34`); ⛔ E2E and web (see below) |

## What the clean build caught (and the incremental build did not)

`make clean && make` failed where an incremental build had been passing:
`settings.cpp` reaches an afterhours header through `settings.h` before
`external.h`, so it got the "none" backend's `TextureType` and would not compile
against the sokol drawing helpers, which now bind textures by
`view_id`/`sampler_id`. `preload.h` had the same ordering. Fixed in `da42e34`.

**Always finish with `make clean && make -j10 all`** after a submodule bump: object
files built before the bump can hide a broken include order for a long time.

Warning count is unchanged from baseline (19 `-Wshadow`) except for 4 new
`-Wswitch-default` warnings, all inside vendored `afterhours/src/plugins/modal.h`,
none in wordproc code.

## E2E cannot run on this machine right now

`make e2e` times out with an empty log, and so does the **pre-bump** binary built from
`c70ee2e` at `/tmp/wp/base` — same hang, same empty log. So this is not the upgrade.

boulder's screen is locked (`CGSSessionScreenIsLocked = true`). The app starts, loads
settings, calls `sapp_run`, and then sits in `-[NSApplication run]` waiting for events
that never come: with the session locked, no frame callback fires, so the E2E runner
never reaches its first frame and never prints. Unit tests are unaffected — they link no
graphics backend.

**To finish verification:** unlock boulder, then
`./tests/run_e2e.sh -r` (and specifically `test/pass_context_menu_*.e2e`, which cover the
menu this upgrade rewrote — they click items by label, so the new geometry is fine, but
they have not been run).

---|------|-----------------|-------|
| T0 | Baseline on old pin | — | ✅ build + 160 tests green |
| T1 | Bump submodule + fix sokol impl TU | — | ⬜ |
| T2 | Adopt `imm::context_menu` | #15 | ⬜ |
| T3 | Focus ring | #5 | ⬜ |
| T4 | Drop shadows on popups | #10 | ⬜ |
| T5 | Toolbar icons via `with_on_draw_fg` | #1 / #9 | ⬜ |
| T6 | Dropdown triangles in-pass | #4 | ⬜ |
| T7 | Access-key underlines in-pass | #2 | ⬜ |
| T8 | Styled labels / font weight | #12 (partial) | ⬜ |
| T9 | Menu + toast animation | #6 | ⬜ |
| T10 | E2E right-click on real input | #14 (partial) | ⬜ |
| T11 | Refresh `docs/afterhours-feature-requests.md` | — | ⬜ |
| T12 | Full verification sweep (unit + e2e + web) | — | ⬜ |

---

## What the 235 commits actually bring

Resolved upstream (was a wordproc workaround):

- **`imm::context_menu(ctx, ep, items, at, open, config)`** plus `menu_list`,
  `dropdown_menu`, `popover`, `MenuItem{label, shortcut, separator, disabled}`,
  anchored overlay placement with edge flipping, dismiss-on-focus-loss. (#15)
- **`with_on_draw_bg(fn)` / `with_on_draw_fg(fn)`** — custom draw inside a
  component's bounds, *during* the render pass, so it respects render layers.
  This is the requested custom-render callback and it also solves icons. (#9, #1)
- **`with_icon(texture, source_rect)` + `with_icon_position(...)`** for real
  texture icons. (#1)
- **`with_shadow(...)` / `with_hard_shadow(...)`**, `Shadow{style, offset, blur, color}`. (#10)
- **Focus ring**: `theme.focus_ring_thickness` / `focus_ring_offset`, rendered by
  `RenderImm`, plus focus arbitration fixes (hover vs keyboard, arrow keys). (#5)
- **Animation**: `AnimationDef`, `AnimTrigger`, `AnimProperty`, `AnimCurve`,
  `HasAnimationState`. (#6)
- **Styled text**: `with_styled_label(std::vector<TextSpan>)` (per-span colour +
  `FontWeight`), `with_font_weight`, wrap-aware measurement, multi-line text that
  measures the same on every backend. (#12, partial — no italic/underline spans)
- **E2E**: `MouseState.right_down` exists now (button 1 is modelled), modifier
  keys can be *held*, double/triple-click selection assertions. (#14, partial —
  still no `right_click` command in `coord_commands`, no `simulate_right_click`)

Also new and worth using: `with_placeholder`, `with_corner_radius(px)`,
`with_wrap`, `with_readonly`, `with_auto_grow`, `with_submit_on_enter`,
`with_min/max_width/height`, per-side borders, `divider`, `hsplit_pane` /
`vsplit_pane`, smooth (eased) scroll views, off-screen culling in scroll
viewports, atomic settings/save writes, `text_area` with selection + clipboard +
word nav.

Still **not** upstream (keep the workaround, keep the request open):

- #2 mnemonic/access-key underline as a label feature (only per-span colour and
  weight exist — no underline)
- #8 tooltip component
- #11 scrollbar visual customization
- #13 grid/table layout
- #14 the E2E `right_click` command + `simulate_right_click()`

---

## T1 — Bump the submodule and fix the sokol impl TU

The bump **compiles clean** (no API drift in `src/`); it fails at *link* only:

```
Undefined symbols for architecture arm64:
  afterhours::graphics::AFTERHOURS_MISSING_sokol_impl_TU__add_one_objcpp_file_that_defines_SOKOL_IMPL_and_includes_afterhours_src_backends_sokol_image_decode_h()
  afterhours::graphics::AFTERHOURS_MISSING_sokol_impl_TU__add_one_TU_that_defines_SOKOL_IMPL_and_includes_afterhours_src_backends_sokol_image_decode_h()
  _metal_capture_render_texture   (from metal_backend::metal_capture_frame)
  _metal_create_system_device     (from metal_backend::metal_init)
```

Upstream moved image decoding to stb and the Metal device/capture path into
`capture_impl.h`, and added a sentinel symbol so a missing impl TU fails
legibly. The fix is in **`src/sokol_impl.mm`** (the one `SOKOL_IMPL` TU):

```cpp
#include <afterhours/src/backends/sokol/image_decode.h>   // stb_image impl + sentinel
#define AFTER_HOURS_USE_METAL
#include <afterhours/src/backends/sokol/capture_impl.h>   // metal_create_system_device, metal_capture_render_texture
```

Reference: `vendor/afterhours/tools/headless_capture/sokol_impl.mm`.

- [ ] Add the two includes to `src/sokol_impl.mm`; check for name collisions with
      wordproc's own `metal_take_screenshot` / `metal_set_window_size` / `metal_minimize_window`
- [ ] Mirror whatever the web TU needs in `src/sokol_impl_web.cpp` (`make web`)
- [ ] `make -j10 all` links; `make -j10 test` still 160/160
- [ ] Commit: submodule pointer + `src/sokol_impl.mm` only

## T2 — Context menu (#15)

`src/ecs/menu_ui_system.h:1144-1252` hand-builds the right-click menu out of
`modal` + `tray` + `button` (~110 lines) against `dialogs.showContextMenu`.
Replace with `imm::context_menu` + `MenuItem::sep()`.

- [ ] Swap the manual popup for `context_menu(ctx, mk(...), items, {x, y}, open)`
- [ ] Keep the existing `Cut / Copy / Paste / Select All` semantics and shortcuts
- [ ] Keep `dialogs.showContextMenu` as the `open` flag so the e2e command still drives it
- [ ] `tests/` + e2e menu tests green

## T3 — Focus ring (#5)

- [ ] Set `focus_ring_thickness` / `focus_ring_offset` in `src/ui/theme.h` to the Win95 look
- [ ] Verify the ring is visible on toolbar buttons and dialog controls
- [ ] Add a unit test asserting the theme carries a non-zero ring

## T4 — Drop shadows (#10)

- [ ] `with_hard_shadow(2, 2, ...)` on menus, dropdowns, context menu, dialogs
- [ ] Confirm no z-order regression with the icon overlay

## T5 — Toolbar icons via custom draw (#1, #9)

`src/ecs/toolbar_overlay_render.h` (370 lines) draws every icon *after* the UI
pass, and **skips drawing entirely when a popup is open** so icons don't punch
through menus. `with_on_draw_fg` runs inside the pass with correct layering.

- [ ] Move `drawToolbarIcon` into an `on_draw_fg` lambda per toolbar button
- [ ] Delete the icon half of the overlay system and the overlap/skip logic
- [ ] Icons stay visible while a menu is open (that is the regression test)

## T6 — Dropdown triangles (#4)

- [ ] Draw the ▼ with `on_draw_fg` on the dropdown trigger; drop the stored
      triangle positions and the overlay pass
- [ ] Consider `imm::dropdown_menu` for the formatting-bar dropdowns

## T7 — Access-key underlines (#2)

Still no upstream mnemonic API, but the underline no longer needs to be an
out-of-pass overlay.

- [ ] Draw the underline in `on_draw_fg` on each menu header / item
- [ ] Retire the rest of `toolbar_overlay_render.h` if nothing is left in it

## T8 — Styled labels (#12, partial)

- [ ] Use `with_styled_label` / `with_font_weight` where wordproc renders
      multi-style UI text (status bar, formatting bar state)
- [ ] Document that the *document canvas* stays custom (italic/underline spans
      are not upstream)

## T9 — Animation (#6)

- [ ] Menu open/close and toast slide/fade via `AnimationDef`
- [ ] Keep it off in test mode so e2e frame counts stay deterministic

## T10 — E2E right-click (#14, partial)

- [ ] Drive the `right_click` e2e command through `MouseState.right_down`
      instead of poking `dialogs.showContextMenu` directly
- [ ] If the injector still cannot press button 1, leave the shim and say so here

## T11 — Refresh the feature-request doc

- [ ] Mark #1, #4, #5, #6, #9, #10, #15 resolved with the API that landed
- [ ] Mark #2, #12, #14 partial with exactly what is still missing
- [ ] Keep #8, #11, #13 open
- [ ] Update the "Current afterhours" line to the new pin

## T12 — Verification sweep

- [ ] `make clean && make -j10 all`
- [ ] `make -j10 test` (expect ≥ 160 cases)
- [ ] `make e2e` / `tests/run_e2e_batch.sh`
- [x] `make clean && make -j10 all` (clean build green after `da42e34`)
- [x] `make -j10 test` — 163 cases / 1702 assertions
- [ ] `make web` — **cannot run here**: no `emsdk/` and no `em++` on this machine.
      `src/sokol_impl_web.cpp` got the image_decode.h include the native TU needed,
      mirroring afterhours' own `examples/web/sokol_impl.cc`, but it is unverified.
- [ ] `make benchmark` / `make e2e` — both run the GUI binary; blocked by the lock
