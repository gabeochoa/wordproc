# Performance Architecture

This document describes Wordproc's performance characteristics and the data-oriented architecture approach.

## Current Architecture (v0.1)

### Text Buffer Storage (AoS - Array of Structures)

Current implementation uses `std::vector<std::string>` where each line is a separate string:

```cpp
class TextBuffer {
  std::vector<std::string> lines_;  // One string per line
  CaretPosition caret_;
  TextStyle style_;
};
```

**Characteristics:**
- Simple to implement and reason about
- Good for small documents (< 10KB)
- Each insertion may allocate memory
- Line iteration requires pointer chasing

### Current Performance

Measured on Apple M1:
- Startup time: ~50-80ms (well under 100ms target)
- Typing latency: < 1ms per character
- File load (1KB): < 10ms

## Baseline — 2026-10-17 (afterhours `d1d7f0e`)

Measured with the existing text-buffer benchmark, `tests/benchmark_text_buffer.cpp`
via `make bench-unit` (Catch2 `[benchmark]` tag, `-O2` test binary), on an
Apple M1 Pro, 10 cores, 32 GB, macOS 27.0.1. Each case ran 3 times; the table
reports the median. These are in-memory `TextBuffer` operations (gap buffer),
not end-to-end keystroke-to-pixels latency — no rendering, layout, or event
loop is involved.

| Benchmark | Workload | Median | Notes |
|---|---|---|---|
| Sequential insert | 10,000 chars at caret | 0.155 µs/char (1.55 ms total) | 0 gap moves, 2 reallocations |
| Random-position insert | 1,000 inserts into 10,000-char doc | 0.284 µs/insert (0.28 ms total) | 1,000 gap moves |
| Sequential backspace | 10,000 deletes | 0.263 µs/delete (2.63 ms total) | |
| Typing burst (typing latency) | 500 keystrokes into 5,000-char doc | 0.148 µs/keystroke (0.074 ms total) | ~6.7M chars/sec capability |
| Line wrap layout | 50,000 chars, 710 lines → 987 wrapped at width 80 | AoS 0.084 ms (0.118 µs/line); SoA 0.010 ms (0.014 µs/line) | SoA ~8× faster |
| `setText` large doc | 100,000 chars → 1,415 lines | 0.037 ms (~2,700 MB/s) | |
| `getText` large doc | 100,000 chars | 0.003 ms (runs ranged 0.003–0.014 ms) | Near timer granularity — treat as "≤ 0.014 ms", not a precise figure |
| SoA line access | 10,000 random accesses, 50,000-char doc | `lineSpan` 0.019 µs/access; `lineString` 0.067 µs/access | SoA ~3.4× faster than copying |

Reading: every typing/latency figure is three orders of magnitude under the
16 ms frame budget. The numbers to beat for any buffer change are the typing
burst (0.148 µs/keystroke) and random-position insert (0.284 µs/insert) —
sequential insert is the easy case (no gap moves).

Not baselined here: scroll at 100K words and cold-start/open time need the GUI
binary (`tests/run_fps_scroll_test.sh`, `tests/run_benchmark.sh`), which cannot
run while the screen is locked, and the text-buffer benchmark does not cover
them. No numbers are claimed for those until they are measured.

## 100K-Word Baseline — 2026-10-17

Measured with four new `[large-doc]` cases in `tests/benchmark_text_buffer.cpp`
via `make bench-unit`, same machine as above (Apple M1 Pro, 32 GB, macOS 27.0.1).
The document is deterministic: a 16-word vocabulary cycled in order, 20 words
per line, no trailing newline — exactly 100,000 words, 5,000 lines, 543,749
chars, verified by `stats().words`. Each case takes the median of 3 runs
inside the bench; the table reports the median of 3 bench invocations.
Unit-level only: in-memory `TextBuffer` operations, no rendering, layout
beyond the wrap functions, or event loop.

| Benchmark | Workload | Median | Notes |
|---|---|---|---|
| Load: `setText` | 543,749 chars → 5,000 lines | 0.085 ms (~6,400 MB/s) | In-memory load only |
| Load: file load | Same doc as `.txt` via `loadTextFileEx` (file I/O + `setText`) | 0.288 ms (~1,890 MB/s) | First run in each process was 3.5–16.9 ms (cold); median unaffected |
| Full layout, SoA | 5,000 lines → 10,000 wrapped at width 80 | 0.063 ms | `layoutWrappedLinesSoA` |
| Full layout, AoS (comparison) | Same | 0.420 ms | Legacy string-copying path, ~6.7× slower than SoA |
| Mid-document edit | `insertChar` + `backspace` at line 2500, fresh buffer per run | 9.1 µs/pair | Includes the full gap move from end-of-doc to the middle; ~0.06% of the 16 ms frame budget |
| Scroll proxy (PROXY) | 100 successive windows × 50 source lines, windowed wrap-count | 0.008 ms total, 0.075 µs/window | **Not real scrolling** — see below |

Reading: nothing here is embarrassingly slow, so **no optimization was done
in this round** (per the rule: one optimization only for an embarrassingly
slow number). The slowest absolute figures are one-time or legacy costs —
file load 0.288 ms (once per open) and AoS layout 0.420 ms (legacy path, not
the one used at scale) — and every figure is under 0.5 ms, over 30× inside
the 16 ms frame budget. The per-keystroke figure that matters, mid-doc
edit at 9.1 µs, is three orders of magnitude under the budget.

Scroll proxy, honestly: there is no unit-level scrolling to measure.
`RenderCache::rebuild` always starts at row 0 (scrolling is a TODO) and
`layoutWrappedLinesSoA` takes no range, so no API lays out one viewport.
The proxy walks successive 50-line windows and applies the same per-line
wrap arithmetic (`ceil(len / 80)`, 1 for empty) without allocating a
`LayoutResult`. It is cross-checked — proxy wrapped count (10,000) equals
the full-layout wrapped count — but it estimates what windowed layout
*would* cost, not what scrolling *does* cost. A real scroll number still
needs the GUI binary.

What's next: (1) real scroll-at-100K-words and cold-start/open numbers from
the GUI binary once the screen is unlocked; (2) investigate the first-run
file-load spike (3.5–16.9 ms on the first `loadTextFileEx` call in a
process) before it matters for open-time claims; (3) if a future edit
workload (e.g. repeated edits with line-offset shifting) ever approaches
the frame budget, that — not these numbers — is the optimization target.

## Perf Round 2 — 2026-10-19 (headless, typing + idle + paged)

Round 1's baseline still stands (first-open wall 130 ms, pageless scroll
0.59 ms/frame, menu +0.24 ms). Round 2 adds idle, typing, and paged /
2-column scroll numbers, fixes what they indicted, and leaves the rest
alone with evidence. Same method as round 1: headless only, `-O2`,
median of 3 runs. This round also shipped the sunken unfocused text-field
fix (commit `9f8dd1a`, Batch 10 polish — not a perf change, not covered
here).

Method notes (read before comparing): `--test-mode --frame-limit=N`
with NO script runs production frames (the E2E `DocumentTextRegistration`
whole-doc walk gates on script mode, not test mode, so it is a no-op
here). Any run WITH a `--test-script` pays ~5 ms/frame of registration
on War and Peace — script-mode numbers are therefore only comparable as
differentials between runs with the same script shape (see "Harness
notes" at the end). Typing is measured as a surcharge per key: 100
`type "a"` commands (1 key per 3 frames, Paged mode, caret mid-document
via GoTo) vs the same script with the types replaced by waits.

### (1) Idle frame CPU

| Doc | Command (plus `--headless --test-mode`) | ms/frame |
|---|---|---|
| Empty | `--frame-limit=300` vs `60`, (236.9−91.8)/240 | 0.61 |
| 100K words, normal vocabulary (5,000 lines × 20 words) | same differential, (226.2−119.7)/240 | 0.44 |
| 100K words, all misspelled (NATO-alphabet words, none in the 1,826-word `en_basic.txt`) | same differential, (1062.5−296.7)/240 | 3.19 |
| War and Peace (566K words, probe) | same differential, (250.5−152.0)/240 | 0.41 |
| `hello.txt` (1 line, probe) | same differential, (169.4−93.0)/240 | 0.32 |

Reading: idle on a normal doc is ~0.4–0.6 ms/frame regardless of size —
nothing walks the document on an idle frame. The 3.19 ms/frame on the
all-misspelled doc is ~2.75 ms/frame of squiggle RENDERING (per squiggle
a `measure_text` of the line prefix plus ~2 `draw_line` calls per 4 px
of word; ~800 squiggles/frame on that doc). Real prose pays ~0 for it
(War idle is 0.41). See "Left alone" for why the render was not changed.

### (2) Typing frame CPU (War and Peace, Paged mode)

| Metric | Before | After fix |
|---|---|---|
| Typing surcharge, paired scripts, 100 keys, mean of 5 interleaved pairs (ms/key, includes the harness artifact below) | 11.04 (median 10.65) | 6.57 (median 6.59) |
| Same, artifact-free estimate of true production typing cost (ms/key) | ~5.0 | ~1 |

Decomposition (micro-bench on the real buffer + bisection in the app,
same 100-key workload): `stats()` is 4.44 ms/call hot (200 iters),
~4.9 ms/key in-app (cold-cache + interleaving), 0.50 ms on the 100K doc,
~0 on empty. Squiggle full-window recheck after a version bump is
0.023 ms. Raw `insertChar`+`backspace` mid-doc is 0.055 ms (the
line-offset shift is O(lines-after-caret) but ~0.03 ms). So before the
fix, a keystroke on War cost ~5 ms, ~4.9 of it `stats()` — nearly a
third of a 60 Hz frame just for the `Words: N` segment.

Fix (commit `07c71d5`): the status bar recomputes `stats()` at most
once per 250 ms of wall time while the buffer keeps changing, but
recomputes as soon as the buffer has been still for 5 frames (settle),
so pauses between keys and the end of any burst show fresh counts. The
Word Count dialog had the same root cause — it called `stats()` every
frame while open — and gets a version gate instead (the document cannot
change under the modal, so it recomputes exactly once per open).
Regression E2E: `pass_status_bar_words_catch_up.e2e` pins the catch-up
(with the interval probed to 60 s it fails: bar stays stale).
`pass_live_word_count.e2e` is untouched and still passes — its waits
are pauses, so the settle rule refreshes within them. `make test` and
`make bench-unit` (incl. the typing-burst guard) still green.

### (3) Scroll: Paged and 2-column (War and Peace)

`--fps-test` auto-scroll (3 lines/frame) + a script that switches the
view first, wall differential over 540 frames (N=590 vs N=50, same
script shape). The script's menu navigation cancels in the differential.

| Mode | ms/frame |
|---|---|
| Pageless (script method; round 1 no-script method: 0.59) | 0.69 |
| Paged (View > Page Layout) | 0.33 |
| 2-column (Paged + Format > Columns… > preset 2; scriptable via the dialog) | below noise floor (see below) |

Paged scroll is CHEAPER than pageless, and the 2-column differential is
indistinguishable from zero: batch 1 gave (804.7−794.6)/540 = 0.02, but
a second batch's walls did not grow with frame count at all (N=50:
1088 ms, N=590: 830 ms, N=2000: 924 ms — run-to-run swing ±300 ms on
this loaded box swamps the signal). Upper bound from the absent
signal: ~0.2 ms/frame. The 2-column view itself was verified by
screenshot (two columns on a small doc; long lines overflow their
column per the documented no-wrap renderer).

### Investigated and left alone

- Paged full-page render (suspect: redrawing all pages' text vs the
  visible page only) — DEAD. `renderTextBuffer` breaks past the text
  area, and paged (0.33) is cheaper than pageless (0.69). No change.
- Squiggle cache on typing (suspect: whole-window recheck per
  keystroke) — already incremental per Fix 3: 0.023 ms/window, and a
  full-window recheck on version change is required by line-shift
  semantics. No change.
- Squiggle RENDERING on fully-misspelled docs (2.75 ms/frame, §1) —
  left alone. It bites only docs where (nearly) every word is flagged
  (e.g. non-English text against the English-only dictionary). Fixing
  it needs a render-side squiggle-position cache whose invalidation
  key must cover zoom, resize, columns, and indents — risk >> benefit
  for a pathological case.
- OnDemand frame mode (`frame_loop::Mode::OnDemand`) — NOT viable.
  Frame requests come only from sokol input events; there is no
  timer-driven request, so caret blink, flash expiry, toast,
  autosave, and E2E ticks (all live in the frame fn) would freeze
  between inputs, and headless drives frames manually so `frame_mode`
  does not apply there anyway. Stays `Continuous` at `target_fps =
  200`. The "skip expensive systems on unchanged frames" alternative
  was also skipped: idle is already 0.44 ms/frame, and a dirty flag
  across all systems is big/risky for small gain.
- 2-column scroll cost — not resolved by this method (noise floor,
  §3). No change; no evidence of a cost to fix.

### Harness notes (for future benches)

- Script-mode frames on War pay ~5 ms/frame of test-only
  `DocumentTextRegistration` (whole-doc `lineString` per frame). Never
  compare a script-mode absolute against a no-script absolute; use
  differentials with identical script shapes.
- `type`/`key` E2E commands cost ~1 extra frame each vs `wait_frames`
  (same nominal wait). Against War that reads as ~4–5 ms/key of
  artifact (the extra frames run registration). It cancels in
  before/after comparisons with identical scripts, but a
  typing-vs-idle differential that mixes command types overstates
  true typing cost. (`wait 0.05` == `wait_frames 3`: tick-equivalent.)
- `samples=` from `--fps-test` is the 120-frame cap when a script is
  also present, not the script's frame count. Screenshots do not flush
  in `--fps-test` runs; use `--test-mode` for probe screenshots.
- Headless loop runs flat-out (~2700 fps no-script); script-mode runs
  ~100–200 fps on War (registration-capped). Wall-time waits
  (throttles, flashes) therefore cover far fewer frames than their
  second count suggests on small docs — size `wait_frames` by wall
  time, not frames.

## Perf Round 3 — 2026-10-19 (profiling build, typing on War and Peace)

Continues Round 2 on the profiling build (`zig build
-Dprofiling=true`, commit `ac5c91d` — it wires the afterhours
collector, `profile_reset`, and `dump_profile`). Same doc, same
paired-typing idea, but the collector now ranks systems per window
instead of inferring from wall differentials. Scripts in
`/tmp/perf3` (not committed): setup is Paged mode + GoTo line
33000 (caret mid-document), then `profile_reset`, then the typing
window, then `dump_profile 40`. Typing window: 100× `type "a"` +
`wait_frames 3`. Control: each `type` replaced by `wait_frames 3`.

Frame accounting (measured, not assumed): `type "a"` occupies 5
frames (1 dispatch + 3 of its own wait + 1 extra frame vs
`wait_frames`), `wait_frames 3` occupies 4. So the typing window
is 902 frames, the control 802 — the 100 extra frames are a
harness artifact (~5.2 ms/frame of script-mode per-frame work:
registration 3.52 + UI bridge 1.33 + editor 0.25 + …), and
`dump_profile`'s `mean` is total/calls, so per-system totals are
`mean × calls` and the artifact-free per-key cost is
`(total_typing − total_control − 100 × control_per_frame) / 100`.
Medians of 3 interleaved pairs; machine load 35–51, so small
residuals (±0.1–0.5 ms/key) are noise.

### (1) Where the typing frame's ~6 ms goes

Round 2's "after fix 6.57 ms/key, artifact-free ~1" was a
dense-cadence artifact: its 1-key-per-3-frames script never let
the status bar's 5-still-frames settle rule fire. At human-like
cadence the true cost is ~4.9 ms/key, ~90% of it one system:

| System | Before: artifact-free ms/key | After: artifact-free ms/key |
|---|---|---|
| `StatusBarSystem` | 4.42 (460 ms total) | ~0.00 (9 ms total) |
| `UIPluginRenderBridge` | 0.18 | −0.27 (noise) |
| `DocumentTextRegistration` | 0.09 | −0.54 (noise) |
| `EditorRenderSystem` | 0.09 | 0.09 (noise) |
| `TextInputSystem` | 0.09 | 0.09 (real insert path, at floor) |
| Others (toolbar, menu, post-update, …) | ~0.00 | ~0.00 |
| **Total artifact-free** | **4.87** | **~0 (median sum −0.8, noise)** |
| Raw incl. 100-extra-frame artifact | 10.09 | 4.44 |

Mechanism, proved by cadence: dense typing (5 frames/key, still
gap < 5) totals 55 ms in `StatusBarSystem`; slow typing (36
frames/key) totals 504 ms — one `stats()` per key each time the
settle rule fires. `stats()` scanned the whole gap buffer
(~4.9 ms on War). A wall-time throttle cannot bound this for
human typing (keys arrive ~200 ms apart, so a 250 ms throttle
still fires about every other key), and the live-update E2E
(`pass_live_word_count`, fresh count within 12 frames) requires
per-pause freshness — so the fix had to make `stats()` itself
cheap, not retime it.

Fix (commit `6ab7b52`): `TextBuffer::stats()` is incremental.
Words / sentences / paragraphs are maintained by the mutators: a
single-char edit changes word starts only at the edit point and
its right neighbour, sentences only for the char itself,
paragraphs only for the touched line(s); a range erase scans only
the erased range; `swapAdjacentLineRanges` needs no update (it
reorders whole lines). `setText` only marks the cache dirty — an
eager rebuild pushed War `setText` 3.2 ms → 10.5 ms and broke the
10 ms load gate (`[perf][load]`, HEAD-verified); the first
`stats()` call or mutator after a `setText` rebuilds once. Lines /
characters stay O(1) live reads. The status-bar
throttle/settle logic is unchanged; it was never the bug.
Regression test: `TextBuffer stats stay exact across edits`
compares `stats()` against a from-scratch scan after every
mutator (inserts, backspace/del incl. newline merges, selections,
replace, undo/redo, swap, TOC insert, lazy-rebuild path).

After the fix, slow-cadence typing totals 36 ms in
`StatusBarSystem` (was 504 ms), and the dense/slow distinction
is gone.

### Investigated and left alone (no second fix)

No second real cost exists: after the fix no system shows a
residual above the noise floor except `TextInputSystem` (+0.09
ms/key, the actual `insertChar` path: gap move + line-offset
shift, at the bench-unit floor).

- Per-keystroke rewrap — dead. The body renderer draws each
  buffer row as one line; wrapping exists only in table-cell
  text (`cell_edit::wrapCellText`, header/footer) and drop-cap
  indent math, not on the per-keystroke path.
- Squiggle recheck — dead. `SpellSquiggleCache` rechecks the
  whole window on a version change, required by line-shift
  semantics (Round 2: 0.023 ms/window); scroll reuses rows.
- Measure-cache misses — dead. `TextMeasureCache` is a
  persistent 4096-entry LRU; the UI bridge's typing residual is
  ±noise (−0.27 ms/key).
- Imm UI rebuild per frame — at floor. `UIPluginRenderBridge`
  costs 1.32 ms/frame idle and typing adds nothing measurable;
  retaining UI across frames is the big/risky change Round 1
  already rejected.

### Guards

`make test` green (9518 assertions, 320 cases, incl. the new
stats case and `[perf][load]`). `make bench-unit` typing burst
0.175 µs/keystroke vs 0.149 on HEAD on the same box — the delta
is the one-time lazy rebuild amortized over the 500-key burst
(warm-cache probe: 0.163 µs/key; `stats()` itself 2 ns/call on
a 6,000-char doc). Zero new clang warnings (old vs new
`text_buffer.cpp` under the app flags — identical output).
Headless E2E spot on the normal build, all exit 0:
`pass_live_word_count`, `pass_status_bar_words_catch_up`,
`e2e_word_count`, `e2e_large_doc_scroll` (War and Peace).

### Vendored fmt (addendum recheck, 2026-10-19)

Wordproc's vendored fmt is `FMT_VERSION 110200` (11.2.0) — same
as mailboy; kart-afterhours still vendors 8.0.1 (kart's
exposure, not wordproc's). A TU shaped like the app
(`-include src/log.h` + `<fmt/compile.h>` `FMT_COMPILE`
consteval path) compiles and runs clean under the system clang
(Apple clang 17.0.0) and the zig-bundled clang (`zig c++`).
**No fmt bump needed.** (A bare `zig c++` probe without
`log.h` fails on undeclared `malloc`/`free` — a probe-shape
artifact, not the consteval break: the app always pre-includes
`log.h`.)


## Perf Lane Baseline — 2026-10-19 (headless, interaction feel)

Owner priority: (1) cold first-open → first interactive frame, (2) scroll
frame time on a 100K+ word doc, (3) menu/popover open latency (target:
visible next frame, no layout jank).

Method (this section and every fix after it): headless only
(`./output/wordproc.exe --headless --test-mode ...`, `make test` /
`make bench-unit`; never windowed, never `tests/run_e2e.sh`). Machine:
Apple M1 Pro, arm64, macOS 27.0.1. Base commit `78417f1`. Each number is
the median of 3 runs of the exact command shown. Wall time is process
wall (`time`-style, includes process start + shutdown); "Startup" is the
app's own `Startup time:` log (main → `app_init` done, before any frame).

### (1) First-open

Trivial script = one screenshot + `wait_frames 1`, `--frame-limit=30`.

| Open | Command (plus `--headless --test-mode --test-script=<trivial> --frame-limit=30`) | Median wall | Median `Startup time` |
|---|---|---|---|
| No file | (no positional arg) | 231.1 ms (227.4, 231.1, 254.5) | 119 ms (101, 119, 124) |
| `war_and_peace.txt` | positional `test_files/public_domain/war_and_peace.txt` (566,334 words, 66,032 lines, 3.2 MB) | 331.9 ms (296.8, 331.9, 390.3) | 105 ms (95, 105, 142) |

`SCOPED_TIMER` breakdown (medians, same runs), top costs first:

| Timer | No file | War and Peace |
|---|---|---|
| `Load fonts` | 59.2 ms | 58.4 ms |
| `font: Gaegu-Bold / Gaegu` | 50.1 ms | 49.5 ms |
| `font: Garamond` | 5.3 ms | 5.3 ms |
| `font: DEFAULT_FONT (Roboto)` | 1.2 ms | 1.2 ms |
| `font: SYMBOL_FONT (Roboto)` | 1.2 ms | 1.2 ms |
| `font: UI_FONT (manual load)` | 1.2 ms | 1.2 ms |
| `Settings load` | 8.1 ms (runs 3.0–14.4, noisy) | 6.1 ms (runs 0.5–9.8, noisy) |
| `Background document preload` | — | 19.4 ms, of which `setText (plain text)` 18.2 ms, `file I/O (readFileRaw)` 1.2 ms |
| `createMenuBar` | 0.055 ms | 0.053 ms |
| `Register all systems` | 0.095 ms | 0.090 ms |
| `Preload and singletons` | 60.0 ms | 59.3 ms |

Reading: fonts are ~50% of `Startup time`, and Gaegu alone is ~85% of
font time (2.8 MB `Gaegu-Bold.ttf`, loaded twice under two names —
`Gaegu-Bold` and `Gaegu` — in one 50 ms timer). Garamond is next at
5.3 ms. Everything else in startup is < 1 ms except noisy `Settings
load`. War-and-Peace load itself is 19 ms of preload (`setText`
18 ms — far above the 0.085–0.11 ms `setText` in `bench-unit` for a
similar-size synthetic doc, so the preload-path `setText` cost is
indicted for investigation, not assumed to be the gap buffer). The
`Startup` timer excludes the frame loop; wall − Startup ≈ 110–225 ms
is process start, 30 frames, screenshot, and shutdown.

### (2) Scroll frame CPU, War and Peace

`--fps-test` auto-scrolls 3 lines/frame. `get_fps()` is **simulated
in headless**: `MetalPlatformAPI::get_frame_time()` returns a fixed
`1/60` when `g_headless`, so the logged `avg_fps=60.00` (all runs) is
not a measurement. The metric is wall/frame differentials at fixed
frame counts, same command, no script (no E2E overhead — see caveat):

| Run | Command | Median wall |
|---|---|---|
| fps-test, 600 frames | `--headless --fps-test --frame-limit=600 test_files/public_domain/war_and_peace.txt` | 2038.3 ms |
| fps-test, 60 frames | `--headless --fps-test --frame-limit=60 ...` (same file) | 369.8 ms |

Scroll frame CPU = (2038.3 − 369.8) / 540 = **3.09 ms/frame** while
scrolling 3 lines/frame through War and Peace. ~5× inside the 16.7 ms
frame budget; not the bottleneck on this machine.

Caveat (measured, not assumed): script-mode frames are NOT production
frames. A run with a `wait_frames 600` script on War and Peace took
9418 ms vs 1330 ms for `wait_frames 60` → 14.98 ms/frame, while the
same script on `hello.txt` took (1304 − 334) / 540 = 1.80 ms/frame.
The ~13 ms/frame war-only surcharge in script mode is the E2E
`DocumentTextRegistration` system (`src/testing/e2e_commands.h`):
its `once()` runs every tick and registers **all 66,032 lines** via
`lineString` per frame, test-only overhead that does not exist in a
production frame. Script-mode absolutes must never be quoted as
frame time; only same-frame-count script-mode *differentials* (menu
open vs closed, same doc) are valid, because the registration cost
cancels.

### (3) Menu latency

Same-frame-count script-mode differentials on War and Peace (the
registration overhead cancels; see caveat above). Scripts in
`/tmp/perf/` (not committed): `idle200.e2e` (`wait_frames 200`),
`menu_open_idle.e2e` (`menu_open Format` + `wait_frames 200`),
`menu_alt.e2e` (4× `menu_open Format` / `key ESCAPE`, 30 frames each
= 240 frames), `idle240.e2e`.

| Run | Median wall (3 runs) | Delta vs idle, same frames |
|---|---|---|
| Idle, 200 frames | 3465.2 ms | — |
| Format menu open, idle 200 frames | 4231.4 ms | +766.2 ms → **+3.83 ms/frame** steady-state with menu open |
| Idle, 240 frames | 3959.0 ms | — |
| Menu open/close alternating, 240 frames | 5191.5 ms | +1232.6 ms → **+5.14 ms/frame** avg incl. open/close transitions |

Reading: the menu does NOT paint free — an open Format menu costs
~+3.8 ms/frame steady-state, and alternating open/close averages
~+5.1 ms/frame. That is inside the frame budget but is real, recurring
per-frame work while a menu is open (candidates: dropdown relayout /
text re-measure per frame, not a one-frame open spike). The "visible
next frame" half of the target is met by construction (`menu_open`
sets `menu.open = true`, rendered next frame); the cost to attack is
the per-frame menu-open overhead, not open latency in frames.

### Typing guard (baseline, `make bench-unit`)

Typing burst: **0.154 µs/keystroke** (0.077 ms / 500 keystrokes into
a 5,000-char doc). Mid-doc edit: 9.29 µs/pair. Any fix must not
regress the typing burst.

### Vendored fmt (addendum check, 2026-10-19)

Wordproc's vendored fmt is already `FMT_VERSION 110200` (11.2.0) —
the same version mailboy bumped to in `95b6965` ("bump vendored fmt
8.0.1 → 11.2.0 (newer-clang consteval break)"). Test-compiled a TU
using `fmt::format` + `FMT_COMPILE` (the consteval path) with the
system clang (Apple clang 17.0.0) and the zig-bundled clang
(21.1.8, `zig c++`): both compile clean, no consteval errors. **No
fmt bump needed.** (Kart-afterhours still vendors fmt 8.0.1 — that
is kart's exposure, not wordproc's.) No PCH / `build.zig` pattern to
copy: wordproc builds with `makefile`, no PCH.

## Fixes — 2026-10-19 (same commands as baseline, medians of 3)

### Fix 1: build the app with `-O2` (was `-O0`) — `makefile`

Root cause behind several baseline numbers: the app binary was built
with **no `-O` flag** (`-O0`), while the test binary already used
`-O2 -g`. `CXXFLAGS_BASE` now starts with `-g -O2`. An `-O0` build
remains available via `make CXXFLAGS_BASE="-O0 -g"`.

| Metric (same command) | Before (`-O0`) | After (`-O2`, default `make`) |
|---|---|---|
| First-open, no file: wall / `Startup` | 231.1 ms / 119 ms | 80.6 ms / 37 ms |
| First-open, War and Peace: wall / `Startup` | 331.9 ms / 105 ms | 143.8 ms / 49 ms (re-measured on the default build; the isolated `-O2` build gave 130.2 / 39) |
| Scroll, War and Peace (fps-test, 600 vs 60 frames) | 3.09 ms/frame | 0.58 ms/frame ((407.1−96.2)/540) |
| Format menu open, steady-state (script-mode differential, War, 200 frames) | +3.83 ms/frame | +0.47 ms/frame |
| `setText` (plain text), War and Peace preload | 18.2 ms | 2.8 ms |
| `Load fonts` (all faces) | 59.2 ms | 3.6 ms (Gaegu 50.1 → 2.6 ms) |

Verification: `make test` — all pass (2909 assertions, 253 cases);
E2E spot-checks headless on the new default build all exit 0:
`e2e_menu_format_fonts.e2e` (font switching), `e2e_large_doc_scroll.e2e`
(War and Peace scroll), `pass_status_bar_text.e2e`.

### Fix 2: parse each startup TTF once, share the fontstash handle — `src/preload.cpp`

`Font` is just a fontstash id, but `load_font(name, path)` re-read and
re-parsed the file on every call (fontstash has no dedup by path).
Roboto was parsed 3× (`DEFAULT_FONT`, `SYMBOL_FONT`, `UI_FONT`) and
Gaegu-Bold 2× (`Gaegu-Bold`, `Gaegu`). Now each file loads once and
the same handle registers under every name. Garamond stays loaded at
startup: at `-O2` it costs 0.45 ms, and deferring it would need
ensure-loaded wiring at every font-switch site for no measurable win
(see "Not worth fixing").

| Timer (medians, 3x, no-file first-open) | Before (Fix 1) | After (Fix 2) |
|---|---|---|
| `Load fonts` | 3.59 ms | 2.24 ms |
| Gaegu (`Gaegu-Bold` + `Gaegu`) | 2.61 ms (2 parses) | 1.61 ms (1 parse) |
| Roboto (`DEFAULT_FONT` + `SYMBOL_FONT` + `UI_FONT`) | ~0.52 ms (3 parses) | 0.21 ms (1 parse) |
| `Startup time` / wall | 37 ms / 80.6 ms | 39 ms / 88.1 ms (within run-to-run noise; the in-app timers are the signal) |

Side benefit: fontstash now holds one copy of the 2.8 MB Gaegu file
instead of two, and one Roboto instead of three.

Verification: headless E2E `pass_font_dialog_pending.e2e` (Font dialog
preview, incl. Gaegu), `e2e_menu_format_fonts.e2e` (Gaegu/Garamond
switching), `pass_toolbar_font_dropdown.e2e` — all exit 0.

### Fix 3: spell squiggles recheck only newly-visible rows on scroll — `src/editor/spell_squiggles.h`, `tests/test_spell_squiggles.cpp`

`SpellSquiggleCache::update()` rechecked the whole visible window on
every scroll frame (visible range changes every frame while scrolling).
It now keeps squiggles per row: scrolling drops rows that left the
window, reuses rows still visible, and checks only the rows that
entered. A buffer-version change (any edit — an edit can split/join
lines and shift later rows) still discards everything and rechecks
the whole window. A `rowsChecked` counter (cumulative rows actually
spell-checked) makes the behavior testable.

| Measurement | Before | After |
|---|---|---|
| Rows checked, 200 scroll steps × 30-row window, stride 3, War and Peace (`/tmp/perf/squiggle_bench.cpp`, `-O2`, medians) | 6,000 (30/step) | 627 (30 + 199×3) — 9.6× fewer |
| Same bench, wall time for 200 steps | 0.21 ms | 0.135 ms |
| End-to-end scroll, fps-test 600 vs 60 frames | 0.58 ms/frame | 0.62 ms/frame — no measurable change (within run-to-run noise) |

Reading: the absolute win is sub-µs/frame on this doc (War and Peace
rows are nearly all correctly spelled, so the per-row check is cheap)
— the fix removes redundant work, it does not move the fps-test
needle. Kept because it is small, safe, and covered by a test that
fails on revert (verified: forcing a full recheck makes the new test
fail `rowsChecked == 4`).

Verification: `make test` all pass (2931 assertions, 254 cases, incl.
the new `Scroll checks only newly-visible rows` case); headless
`e2e_large_doc_scroll.e2e` exits 0.

### Final numbers (all three fixes, same commands, medians of 3)

| Metric | Baseline (`-O0`, `78417f1`) | Final (Fixes 1–3) |
|---|---|---|
| First-open, no file: wall / `Startup` | 231.1 ms / 119 ms | 129.6 ms / 53 ms |
| First-open, War and Peace: wall / `Startup` | 331.9 ms / 105 ms | 133.5 ms / 49 ms |
| Scroll, War and Peace (fps-test, 600 vs 60 frames) | 3.09 ms/frame | 0.59 ms/frame ((427.3−110.1)/540) |
| Format menu open, steady-state (script-mode differential, War, 200 frames) | +3.83 ms/frame | +0.24 ms/frame ((1303.6−1255.8)/200) |
| Typing guard (`make bench-unit` typing burst) | 0.154 µs/keystroke | 0.160 µs/keystroke (median of 3 re-runs: 0.151–0.183; no regression — no fix touches the typing path) |

Caveat on variance: this box ran a load average of 15–27 during the
lane (other lanes active), so individual runs swing widely (e.g.
`Startup` 31–253 ms across all runs). Every number above is a median
of 3 runs of the exact command; the in-app `SCOPED_TIMER` values are
the least noisy signal for the small fixes.

### Not worth fixing (investigated, evidence in hand)

- **Menu re-architecture (dropdown caching).** The menu paints next
  frame by construction (`menu_open` sets the flag, the dropdown
  renders the next frame). Steady-state open cost is ~0.5 ms fixed +
  ~30 µs/item in immediate-mode rebuild (~2.3 ms/frame for the
  largest, 60-item Format menu at `-O0`; +0.24 ms/frame at `-O2`).
  `TextMeasureCache` is a persistent LRU — no per-frame re-measure
  misses. Caching dropdown UI across frames would mean retaining
  entities in vendored afterhours code: big change, risk >> benefit
  for a sub-ms steady cost inside the frame budget. At floor.
- **Deferring Garamond at startup.** At `-O2` the Garamond load is
  0.45 ms. Deferring it needs ensure-loaded wiring at every
  font-switch site (menu, Ctrl+2, toolbar, font dialog) for a gain
  inside measurement noise. The font dropdown never requests the
  Garamond face (its labels render in the UI font), and switching
  fonts only sets `style.font` metadata — so a lazy load would have
  no first-use trigger in current code anyway.
- **Per-frame rewrap in `renderTextBuffer`.** There is no wrapping:
  each buffer line draws as one line; the render loop iterates only
  visible rows (breaks past the text area). `RenderCache`
  (`text_layout.h`) exists but has no callers on the render path —
  dead scaffolding, not a per-frame cost. No fix.
- **Spell squiggle recheck beyond Fix 3.** One candidate was
  "recheck only the edited row on typing": an edit anywhere can
  split/join lines and shift later rows, so a version change must
  recheck the whole window. Fix 3 (scroll-only incremental) is the
  safe form.
- **Status-bar `stats()` O(n) per frame.** Already version-cached
  (`status_bar_system.h`: recompute only when `buffer.version()`
  changes). The only other `stats()` call is the Word Count dialog,
  gated on the dialog being open. No remaining per-frame call.
- **`setText` 18 ms preload cost.** Fully explained by `-O0` +
  line count: at `-O2` it is 2.8 ms for 66,032 lines (~40 ns/line,
  same order as the 20 ns/line bench-unit figure for a 5,000-line
  doc). No algorithm change needed.
- **`DocumentTextRegistration` per-frame whole-doc registration**
  (~5 ms/frame on War and Peace in script mode at `-O2`, ~13 ms at
  `-O0`). Real, but test-mode-only: it early-returns in production,
  so real users pay nothing. It is also load-bearing for E2E:
  `app_frame` clears the visible-text registry every frame and
  `expect_text` searches that registry, so the registration is what
  makes off-screen document text assertable. Version-gating it
  would silently break off-screen `expect_text`; gating it on
  "a text assertion is pending" depends on fragile runner ordering.
  Left as-is; script-mode absolutes must never be quoted as frame
  time (see the baseline caveat).
- **Vendored fmt bump.** Already 11.2.0 (mailboy's fix version);
  compiles clean under Apple clang 17 and zig-bundled clang 21.1.8
  including the `FMT_COMPILE` consteval path. No change.
- **Build hygiene note (not fixed):** test objects are compiled
  without `-MMD`, so editing a header does not rebuild the test
  binary — a stale test binary can silently pass. (Hit during this
  lane's red-check: had to `rm` the test object to force a rebuild.)
  App objects do track headers.

## Perf Round 4 — 2026-10-19 (headless, distributions)

Base: `0095fe6` plus nothing (Round 4 starts from a clean tree).
Machine: Apple M1 Pro, macOS 27.0.1, load average ~80 during the
lane (other lanes active), so cross-process wall times are noisy —
the in-app timers and the in-process sampler below are the signal,
not wall medians. No behavior change ships in this round: every
suspect measured at or near floor, and the two real costs found
(one-time menu/tooltip warm-up, pathological-doc squiggle measure)
are cheaper to leave than to fix. What does ship is the sampler
(commit 1), a tooltip-test coordinate fix (commit 2), and this
section (commit 3).

### Method: the frame-time sampler

Wall-time differentials between processes are unusable on a loaded
box: 60-frame bucket differentials went NEGATIVE (spawn/scheduling
jitter swamps sub-ms frame costs). New in `src/main.cpp`,
headless only, off by default:

- `--frame-time-log`: times each `app_frame()` with `steady_clock`
  in the headless loop, skips the first 5 warmup frames, and at
  exit prints `n / mean / p50 / p95 / p99 / max` plus the 5
  slowest frames with their loop indices (so a spike can be
  attributed to the script step on that frame).
- `WORDPROC_FRAME_TIMES=1`: logs every frame's ms, for exact
  single-frame reads (e.g. the first frame after `menu_open`).
- New `SCOPED_TIMER`s: `graphics::init`, `Frame loop`,
  `Cleanup + shutdown`, `E2E runner init + tail`.

Caveat: the sampler's `max` includes a finish-frame artifact —
on script finish, `printResults` + `Settings::write_save_file`
run inside the last `app_frame`; on `--frame-limit` runs the FPS
summary prints in the last frame. Treat `p95`/`p99` as the jank
figures and `max` as an upper bound with that caveat. All
distributions below are medians across runs unless stated.

### (1) Cold open — breakdown, no fix

Final-binary cold open, trivial script (screenshot + `wait_frames 1`),
medians of 5:

| Open | Wall | `Startup` |
|---|---|---|
| No file | 100.0 ms (83–114) | 41 ms (35–47) |
| War and Peace | 155.9 ms (149–178) | 41 ms (38–45) |

Same ballpark as the prior round's 130/133 ms wall; the difference
is box load, not code (no behavior change this round). Breakdown
(medians, no-file; War in parentheses where different):

| Piece | Median |
|---|---|
| `graphics::init` (Metal device + sokol_gfx + fontstash atlas + offscreen texture) | 30.4 ms (War 33.6) |
| Frame loop (screenshot PNG ≈ 25 ms + ~2 frames) | 34.3 ms (War 78.0, incl. ~45 ms script-mode registration) |
| `Settings load` | 2.9 ms (War 3.2; min 2.3 — the rest is scheduler noise) |
| `Load fonts` (Gaegu ~1.5, Garamond ~0.4, Roboto ~0.2) | 2.2 ms (War 2.1) |
| `Preload and singletons` | 2.4 ms |
| `Cleanup + shutdown` | 3.9 ms (War 2.2) |
| `Entity + component setup` (incl. dictionary constructor) | 0.5 ms |
| `createMenuBar` / `Register all systems` / `E2E runner init + tail` | ~0 / ~0 / 0.1 ms |
| `Background document preload` (War; `setText` ~3 + I/O ~1) | 5.9 ms, overlapped; `Collect` 0.0 ms |
| Spawn / dyld / exit residual (wall − Startup − Frame loop − Cleanup) | ~15 ms; benchmark-mode in-process time is 0.004 ms, so the floor is spawn + scheduling, not app code |

Candidates evaluated, all left alone:

- `AFTER_HOURS_ENABLE_E2E_TESTING` IS compiled in
  (`build.zig` `-D` flag — the brief's "compiled in always" is
  correct, my first grep missed it via a literal-mode search).
  Cost ≈ 0: E2E systems register only when the runner has
  commands (`main.cpp`), so production frames pay only a branch
  per input call. Cannot gate it out — E2E needs it.
- Template / dictionary / spell init at startup: none exists.
  Templates load on demand; the dictionary constructor runs
  inside the 0.5 ms entity setup; spell check is windowed per
  frame. Nothing to defer.
- Fonts: `FontLoader::loadStartupFonts` is 0.0 ms (bookkeeping
  only). Gaegu (default document font, ~1.5 ms) cannot be
  deferred — the first frame needs the default face. Garamond
  0.4 ms stays, per the prior round's decision.
- Validation registration: already test-gated (registered only
  when `!testModeEnabled`). Nothing to change.
- `graphics::init` (~30 ms) is vendored Metal setup, required
  for the first frame. At floor; not app-side.
- Test-only overhead inside the wall number: the startup
  screenshot + the script's screenshot + `DocumentTextRegistration`
  (~45 ms on War). Production first-open is ~50–70 ms below
  the test-mode wall.

Conclusion: no app-side cold-open fix with a measurable win —
the wall is infra + harness + scheduler noise.

### (2) Scroll smoothness — no spike on War and Peace

Sampler, War and Peace, `--fps-test` (3 lines/frame), 300 frames,
medians of 5 runs:

| Run | Mean | p50 | p95 | p99 | Max |
|---|---|---|---|---|---|
| Pageless, production frames | 0.44 | 0.44 | 0.77 | 1.21 | 1.38 |
| Paged, script-mode | 5.16 | 5.11 | 5.73 | 6.16 | 6.65 |
| Pageless, script-mode control | 5.23 | 5.18 | 5.67 | 6.17 | 6.42 |

Paged ≈ pageless in script mode (both dominated by the ~5 ms
test-only registration) — no paged penalty. A final-binary
single run gave mean 0.48 / p50 0.46 / p95 0.93 / p99 1.26 /
max 3.49 (max includes the finish-frame artifact). Interleaved
idle/scroll pairs both drifted upward with wall-clock time, so
no idle-vs-scroll delta is claimed.

Suspects, all hunted:

- Squiggle recheck bursts on page jump — DEAD. Five `PAGEDOWN`
  jumps (script-mode): p50 5.59 / p95 5.85, within 0.2 ms of
  the script-mode steady control — no spike. Fix 3's
  incremental rows already hold.
- Status-bar string formatting — DEAD. `StatusBarSystem`
  0.01 ms mean in the profiling build (War scroll and
  misspelled runs alike) — the per-frame `std::format` +
  `localtime`/`strftime` is ~10 µs. Caching the left/right
  strings would save ~nothing. Left alone.
- Layout / UI tree rebuild — DEAD. `LayoutUpdateSystem`
  0.00 ms; the whole production frame is 0.44 ms, so the imm
  UI rebuild is inside that. Retaining UI across frames means
  retained entities in vendored code — rejected in prior
  rounds for the same reason. Left alone.
- Squiggle RENDER on all-misspelled docs — REAL but
  pathological, steady (not a spike), and inside budget.
  Synthetic doc (5,000 lines, every word flagged):
  idle mean 4.68 / scroll mean 4.98 vs War 0.44. Profiling:
  `EditorRenderSystem` 4.22 ms of the ~4.8 ms total.
  Attribution via a temporary env-gated probe (removed):
  full squiggles 4.55 ms mean; skipping only the
  `draw_line` calls 4.39 ms (draw ≈ 0.16 ms); skipping
  measures + draw 0.80 ms (measure ≈ 3.6 ms). So the cost
  is the 2× uncached fontstash prefix `measure_text` per
  squiggle, not the wavy-line drawing. Left alone: it bites
  only docs where nearly every word is flagged, the total
  is still 3.5× inside the 16.7 ms budget, and the fix
  (segmented/incremental measurement, or a squiggle-position
  cache) is approximate or needs the invalidation key Round 2
  rejected (zoom, resize, columns, indents). The attribution
  is now precise for a future round.

### (3) Menu / tooltip latency

Per-frame log (`WORDPROC_FRAME_TIMES=1`), War and Peace,
script-mode (steady frame ≈ 5.4 ms, of which ~5 ms is
test-only registration):

- Format menu, first open: steady frames ~5.4; open frame
  (#23, three runs) 7.41 / 7.33 / 7.45 → **+2.0 ms over
  steady**; next frame 5.7–6.0 (settling); steady-open p50
  5.75 vs closed 5.42 → +0.33 ms steady, matching prior
  rounds. Production-equivalent open frame ≈ 2.4 ms absolute.
- Scaling: Help (small menu) open frame +1.4 ms — the bulk
  is a fixed one-time warm-up; Format adds only +0.6 ms for
  ~30 extra items (~20 µs/item).
- Reopen: a second Format open (after `ESCAPE`) shows NO
  spike (≈ steady) — the cost is one-time (measure-cache
  fill + glyph rasterization + first dropdown build), not
  per-open.
- Decision: no fix. At the ≤ ~2 ms-over-steady target, and
  2.4 ms absolute is 7× inside the frame budget. Startup
  warm-up of menu labels would tax cold open (owner
  priority #1) to save a one-time 2 ms frame; retained
  dropdown UI is vendored retained UI (rejected). The
  ">2× steady" reading is not the binding one: against a
  0.44 ms production steady frame, any absolute +2 ms
  spike is >2× — the absolute budget is what matters.
- Tooltip first paint: hover at (110, 76), empty doc,
  production-ish frames (~0.35 ms steady): first-paint
  frame (#32, the 0.5 s delay expiring) 1.50 / 1.46 ms vs
  steady ~0.35–0.45 → **+1.0–1.1 ms, one frame,
  absolute ~1.5 ms**. A/B with a dead coordinate (80, 76)
  shows frame 32 at 0.49 / 0.45 ms — the spike is the
  tooltip paint, not the frame number. One-time per tooltip
  string (measure cache + glyph raster persist). No fix,
  same reasoning as menu. The 0.5 s delay itself is
  intentional (`tooltip_math::HOVER_DELAY_SECONDS`).
- Test fix: `pass_p0_tooltip.e2e` hovered (79, 76), which
  lands in the 1px gap between toolbar buttons — its
  after-delay screenshot never showed a tooltip (0
  pale-yellow pixels). A hover sweep proved it: x=80 →
  0 px; x=110 ("Print Preview") → ~590 px. Coordinate
  fixed to (110, 76); the after-delay screenshot now shows
  the tooltip (verified visually). With the old coordinate
  the screenshot is tooltip-less (red-on-revert in the
  screenshot sense: the test is screenshot-based, it does
  not assert pixels).

### Vendored fmt (addendum recheck, 2026-10-19)

Still `FMT_VERSION 110200` (11.2.0). An app-shaped TU
(`log.h` + `FMT_COMPILE` consteval path) compiles clean
under Apple clang 17.0.0 and the zig-bundled clang. **No
fmt bump needed.**

### Guards

`make test` green (9858 assertions, 333 cases). `make
bench-unit` green; typing burst 0.160 µs/keystroke
(6.26M chars/s — no regression vs 0.154–0.183 in prior
rounds). Zero new clang warnings (forced recompile of
`src/main.cpp`, grep clean). `scripts/check_e2e.py` and
`scripts/check_no_raylib.sh` ok on the changed test.
Headless E2E spot (direct binary, never `run_e2e.sh`), all
exit 0: `e2e_large_doc_scroll`, `e2e_large_doc_menus`,
`e2e_basic_typing`, `e2e_menu_format_fonts`,
`pass_status_bar_words_catch_up`, `pass_live_word_count`,
`pass_spelling_dialog`, `pass_p0_tooltip`.

## Perf Round 5 — 2026-10-19 (headless, cached text measurement)

Target from Round 4: on an all-misspelled document, frame cost ~4.7 ms,
~3.6 ms of it uncached fontstash prefix measures in the spell-squiggle
render path (vs 0.16 ms of actual squiggle drawing). Normal docs
unaffected (0.44 ms).

### What was wrong

`renderTextBuffer` (`src/ecs/render_system.h`) called
`afterhours::graphics::measure_text` (fontstash `fonsTextBounds` via
the active font) for every squiggle's two line prefixes, plus
selection / caret / line-width / segment measures, every frame.
On an error-dense page that is ~1,600 fontstash walks per frame for
text that has not changed. Two near-miss caches already existed and
neither could serve this path:

- afterhours `TextMeasureCache` answers *advance* widths through a
  `Font`; the document renderer measures *ink bounds* through the
  active font. Swapping one for the other would move every squiggle,
  caret, and selection edge by the side bearings.
- afterhours `measure_memo` is wired only into the `Font`-taking
  `measure_text` overload, not into the backend's
  `graphics::measure_text` the renderer calls — so the render path
  was entirely uncached.

### Fix (one commit)

- New `MeasureCache` (`src/editor/measure_cache.h`, next to the
  squiggle cache): bounded LRU, 4096 entries, keyed on (font key,
  font size, text). Hits return the exact integer the uncached call
  would return. The map key carries two independent text hashes,
  and a hit still verifies the stored full string — a hash
  collision can only evict an entry, never serve a wrong width.
  Buffer edits need no invalidation: widths are keyed on the text
  itself.
- New `cachedMeasureText` wiring (`src/ecs/cached_measure.h`): the
  font key folds in the active fontstash id and the DPI scale —
  everything `graphics::measure_text` depends on besides (text,
  size). A font reload mints a new fontstash id and a display move
  changes the scale, so both land in a different key rather than
  a stale hit. One process-wide cache; the render path is
  single-threaded.
- All 27 `graphics::measure_text` call sites in the render path
  (`render_system.h`: body, header/footer, tables, line numbers,
  ruler; `menu_ui_system.h`: chip-popup caret x) now go through
  it — squiggle, caret, selection, comment/chip highlights, and
  segment measures alike.
- Squiggle and caret sites pass `string_view` slices instead of
  `substr` copies: a copy per squiggle (2 per error) would allocate
  ~1,600 strings a frame on an error-dense page, dwarfing the
  cache hit (measured: idle 1.04 → 0.91 ms on the identical-lines
  doc from this change alone).

### Results (frame-time sampler, medians of 3 runs)

| Doc / mode | Before p50 (mean) | After p50 (mean) | After vs War |
|---|---|---|---|
| Misspelled, identical lines, idle | 4.72 (4.73) | 0.91 (0.93) | 2.0× |
| Misspelled, identical lines, scroll (fps-test) | 5.01 (5.01) | 0.99 (1.02) | 2.3× |
| Misspelled, varied lines, idle | 4.05 (4.04) | 0.84 (0.87) | 1.9× |
| Misspelled, varied lines, scroll | 4.04 (4.06) | 1.23 (1.27) | 2.8× |
| Misspelled, varied lines, typing (10-key script, same script both sides) | 4.59 (4.58) | 1.31 (1.36) | — |
| War and Peace, idle (control) | 0.57 (0.58) | 0.45 (0.46) | — |
| War and Peace, scroll (control) | 0.42 (0.42) | 0.44 (0.44) | — |

The varied-lines doc is new this round (100,000 distinct `zx…`
nonsense words, 5,000 lines, verified 0 in `en_basic.txt`): it is
the honest case for a *persistent* cache — a per-frame-only cache
would score the same on Round 4's identical-lines doc but nothing
here, since no prefix repeats within a frame on varied text. The
control moved within box-load variance (0.57 → 0.45 idle); no
regression. Scroll on the varied doc stays at 2.8×: the residual
is first-sight measurement of newly scrolled-in rows (~120 fresh
prefixes per 3-line scroll frame, ~2–3 µs each) plus the spell
recheck of those rows — irreducible under pixel-identical output,
since ink bounds are not additive across words (a prefix cannot
be built from word widths without shifting positions), which is
why the identical-lines doc, whose prefixes are already cached,
scrolls at 2.3×. Typing-script absolutes include the test-only
registration artifact; only the before/after pair with the
identical script is comparable.

### Pixel-identical squiggles

Before/after screenshots of the same varied misspelled page:
17,883 pure-red pixels in both; per-row red-pixel runs identical.
The raw diff is 279 pixels, all inside the status-bar clock box
(`06:56 PM` → `07:03 PM` — the two runs were 7 minutes apart);
excluding that time-dependent box, the diff is 0 pixels.

### Guards

`make test` green (9906 assertions, 336 cases, incl. new
`tests/test_measure_cache.cpp`: exact-width caching across
fontKey/size/text changes, LRU eviction + recency refresh,
`clear()`). `make bench-unit` typing burst 0.172 µs/keystroke
vs 0.160 in Round 4 — within the 0.151–0.183 run-to-run range;
the fix touches only the render path. Zero new warnings
(app build grep-clean). Headless E2E spot (direct binary, never
`run_e2e.sh`), all exit 0: `pass_spell_squiggles`,
`pass_columns_squiggle`, `pass_spell_common_words`,
`pass_basic_typing`, `pass_selection`,
`pass_p0_mouse_selection`, `pass_shift_arrow_selection`,
`e2e_large_doc_scroll`. `scripts/check_e2e.py` and
`scripts/check_no_raylib.sh` ok (no E2E files changed).

### Vendored fmt (addendum recheck, 2026-10-19)

Still `FMT_VERSION 110200` (11.2.0) — kart-afterhours still
vendors 8.0.1. An app-shaped TU (`log.h` + `FMT_COMPILE`
consteval path) compiles and runs clean under Apple clang
17.0.0 and the zig-bundled clang (zig 0.16.0). **No fmt bump
needed.**

## Planned SoA Architecture (v0.2)

### Contiguous Character Storage

Replace per-line strings with a contiguous buffer and offset table:

```cpp
struct TextBufferSoA {
  // Contiguous character storage
  std::vector<char> chars_;  // All document text
  
  // Line metadata (separate arrays = SoA)
  std::vector<size_t> line_offsets_;   // Start of each line in chars_
  std::vector<size_t> line_lengths_;   // Length of each line
  
  // Alternatively: Gap buffer for efficient insertion
  std::vector<char> gap_buffer_;
  size_t gap_start_, gap_end_;
};
```

**Expected Improvements:**
- Reduced memory fragmentation
- Better cache locality for scanning/rendering
- Single allocation for document load
- Fewer allocations during typing

### Gap Buffer Variant

For editing-heavy workloads, a gap buffer minimizes copies:

```
[Hello, ][        ][World!]
         ^gap^
```

Insertions at cursor position are O(1) instead of O(n).

### Piece Table Alternative

For undo/redo, a piece table may be more suitable:

```cpp
struct Piece {
  enum Source { Original, Add };
  Source source;
  size_t start, length;
};

struct PieceTable {
  std::string original_;          // Original file content (immutable)
  std::string add_buffer_;        // All additions (append-only)
  std::vector<Piece> pieces_;     // Logical document order
};
```

## Rendering Path

### Current (per-frame relayout)

```cpp
void render() {
  for (auto& line : buffer.lines()) {
    DrawText(line.c_str(), x, y, fontSize, BLACK);
    y += lineHeight;
  }
}
```

### Planned (cached layout)

```cpp
struct CachedLayout {
  std::vector<GlyphRun> runs_;    // Pre-measured glyph runs
  std::vector<LineLayout> lines_; // Line break positions
  bool dirty_ = true;
};

void render() {
  if (layout.dirty_) {
    recomputeLayout();
  }
  for (auto& run : layout.runs_) {
    DrawGlyphRun(run);  // Use cached measurements
  }
}
```

## Benchmarking

### Test Harness

Run `make benchmark` to measure load times:

```bash
$ make benchmark
=== Wordproc Load-Time Benchmark ===
Target: <= 100ms cold start

  hello.txt                          62 bytes    52 ms  PASS
  lorem.txt                        1337 bytes    55 ms  PASS
  multiline.txt                     512 bytes    51 ms  PASS

Report saved to: output/benchmarks/load_times_YYYYMMDD_HHMMSS.csv
```

### Metrics Tracked

| Metric | Target | Current |
|--------|--------|---------|
| Cold start (empty) | <= 100ms | ~50ms |
| Cold start (1KB file) | <= 100ms | ~55ms |
| Typing latency | <= 16ms | < 1ms |
| Render frame time | <= 16ms | < 5ms |

## Migration Path

1. **Phase 1 (current):** AoS with per-line strings
   - Functional and tested
   - Adequate for documents < 100KB

2. **Phase 2:** Replace with gap buffer
   - Better typing performance for large docs
   - Minimal API changes

3. **Phase 3:** Add cached layout
   - Avoid per-frame text measurement
   - Required for smooth scrolling

4. **Phase 4:** Consider piece table
   - If undo/redo becomes a bottleneck
   - Good for collaborative editing

## Memory Layout Visualization

### Current (AoS)

```
TextBuffer object:
  [ptr] -> lines_[0]: "Hello, World!\0"
  [ptr] -> lines_[1]: "Second line\0"
  [ptr] -> lines_[2]: "Third line\0"
  
Memory: Scattered heap allocations
```

### Target (SoA)

```
TextBufferSoA object:
  chars_: "Hello, World!\nSecond line\nThird line\0"
  offsets_: [0, 14, 26]
  lengths_: [13, 11, 10]
  
Memory: Single contiguous allocation + two small arrays
```

## References

- [Data-Oriented Design](https://www.dataorienteddesign.com/dodbook/)
- [Gap Buffer](https://en.wikipedia.org/wiki/Gap_buffer)
- [Piece Table](https://www.cs.unm.edu/~crowley/papers/sds.pdf)
- [Xi Editor Rope Science](https://xi-editor.io/docs/rope_science_00.html)

## Direction (Gabe, 2026-10-16)

Rebuilding Word 95 using the afterhours library. Speed and performance is the name of the game:
instant-start editor, fast text layout and editing at large document sizes, Word 95-style UI chrome.
Every perf change reports before/after numbers from the benchmark suite (typing latency,
scroll at 100K words, open time). No perf claim without a measured number.
