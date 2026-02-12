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
