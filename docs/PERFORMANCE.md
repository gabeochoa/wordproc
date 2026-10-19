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
