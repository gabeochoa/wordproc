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
