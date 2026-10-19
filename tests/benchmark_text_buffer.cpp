#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>

#include "../src/editor/document_io.h"
#include "../src/editor/text_buffer.h"
#include "../src/editor/text_layout.h"
#include "catch2/catch.hpp"

// Benchmark utilities
namespace bench {

struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint start_;

    Timer() : start_(Clock::now()) {}

    double elapsedMs() const {
        auto end = Clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    double elapsedUs() const {
        auto end = Clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }
};

// Generate random text for benchmarks
std::string generateText(std::size_t chars, std::size_t avg_line_length = 60) {
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<int> char_dist('a', 'z');
    std::uniform_int_distribution<std::size_t> line_dist(20,
                                                         avg_line_length * 2);

    std::string result;
    result.reserve(chars);

    std::size_t line_len = line_dist(rng);
    std::size_t current_line = 0;

    for (std::size_t i = 0; i < chars; ++i) {
        if (current_line >= line_len) {
            result.push_back('\n');
            current_line = 0;
            line_len = line_dist(rng);
        } else {
            result.push_back(static_cast<char>(char_dist(rng)));
            ++current_line;
        }
    }

    return result;
}

// Report structure for benchmark results
struct BenchmarkResult {
    std::string name;
    std::size_t data_size;
    std::size_t iterations;
    double total_time_ms;
    double per_op_us;
    std::size_t allocations;  // From PerfStats
};

void printResult(const BenchmarkResult& r) {
    std::printf("  %-30s: %8.3f ms total, %8.3f us/op (%zu ops)\n",
                r.name.c_str(), r.total_time_ms, r.per_op_us, r.iterations);
}

// Median of 3 runs: each run is one timed execution of `fn` (setup must
// live outside the timed region, inside `fn` before it starts its Timer,
// or outside `fn` entirely for state reused across runs). Returns the
// median plus the three raw runs so noise is visible.
struct MedianResult {
    double median_ms = 0.0;
    std::array<double, 3> runs_ms{};
};

template <typename F>
MedianResult medianOf3Runs(F&& fn) {
    MedianResult out;
    for (std::size_t i = 0; i < out.runs_ms.size(); ++i) {
        out.runs_ms[i] = fn();
    }
    std::array<double, 3> sorted = out.runs_ms;
    std::sort(sorted.begin(), sorted.end());
    out.median_ms = sorted[1];
    return out;
}

void printMedianRuns(const char* label, const MedianResult& m) {
    std::printf("  %s: median %.3f ms (runs %.3f, %.3f, %.3f ms)\n", label,
                m.median_ms, m.runs_ms[0], m.runs_ms[1], m.runs_ms[2]);
}

// Deterministic 100,000-word document: 16-word vocabulary cycled in order,
// 20 words per line (~110 chars/line, so lines wrap at width 80), no
// trailing newline. Exactly 100,000 words, 5,000 lines. No RNG — the
// same bytes every run, so results are comparable.
std::string generateWordDocument() {
    static constexpr const char* kWords[] = {
        "the", "quick", "brown", "fox", "jumps", "over",
        "lazy", "dog", "and", "runs", "through", "green",
        "fields", "under", "bright", "sky",
    };
    constexpr std::size_t kWordCount = 100000;
    constexpr std::size_t kWordsPerLine = 20;

    std::string result;
    result.reserve(600000);
    for (std::size_t w = 0; w < kWordCount; ++w) {
        if (w != 0) {
            result.push_back((w % kWordsPerLine == 0) ? '\n' : ' ');
        }
        result.append(kWords[w % 16]);
    }
    return result;
}

}  // namespace bench

// ============================================================================
// INSERT BENCHMARKS
// ============================================================================

TEST_CASE("Benchmark: Sequential character insert", "[benchmark][insert]") {
    const std::size_t NUM_CHARS = 10000;

    TextBuffer buffer;
    buffer.resetPerfStats();

    bench::Timer timer;
    for (std::size_t i = 0; i < NUM_CHARS; ++i) {
        buffer.insertChar('a' + static_cast<char>(i % 26));
    }
    double elapsed = timer.elapsedMs();

    auto stats = buffer.perfStats();

    std::printf("\n=== Sequential Insert Benchmark ===\n");
    std::printf("  Characters inserted: %zu\n", NUM_CHARS);
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Per-char: %.3f us\n", (elapsed * 1000.0) / NUM_CHARS);
    std::printf("  Gap moves: %zu\n", stats.gap_moves);
    std::printf("  Buffer reallocations: %zu\n", stats.buffer_reallocations);

    // Verify correctness
    REQUIRE(buffer.lineCount() == 1);
    REQUIRE(buffer.getText().size() == NUM_CHARS);

    // Performance assertion: should be fast
    REQUIRE(elapsed < 100.0);  // Less than 100ms for 10k inserts
}

TEST_CASE("Benchmark: Random position insert", "[benchmark][insert]") {
    const std::size_t INITIAL_SIZE = 10000;
    const std::size_t NUM_INSERTS = 1000;

    std::string initial = bench::generateText(INITIAL_SIZE);
    TextBuffer buffer;
    buffer.setText(initial);
    buffer.resetPerfStats();

    std::mt19937 rng(123);
    std::uniform_int_distribution<std::size_t> row_dist(0,
                                                        buffer.lineCount() - 1);

    bench::Timer timer;
    for (std::size_t i = 0; i < NUM_INSERTS; ++i) {
        std::size_t row = row_dist(rng);
        LineSpan span = buffer.lineSpan(row);
        std::uniform_int_distribution<std::size_t> col_dist(0, span.length);
        std::size_t col = col_dist(rng);

        buffer.setCaret({row, col});
        buffer.insertChar('X');
    }
    double elapsed = timer.elapsedMs();

    auto stats = buffer.perfStats();

    std::printf("\n=== Random Insert Benchmark ===\n");
    std::printf("  Initial size: %zu chars\n", INITIAL_SIZE);
    std::printf("  Inserts: %zu\n", NUM_INSERTS);
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Per-insert: %.3f us\n", (elapsed * 1000.0) / NUM_INSERTS);
    std::printf("  Gap moves: %zu\n", stats.gap_moves);

    REQUIRE(elapsed < 50.0);  // Less than 50ms for 1k random inserts in 10k doc
}

// ============================================================================
// DELETE BENCHMARKS
// ============================================================================

TEST_CASE("Benchmark: Sequential backspace", "[benchmark][delete]") {
    const std::size_t DOC_SIZE = 10000;

    std::string text = bench::generateText(DOC_SIZE);
    TextBuffer buffer;
    buffer.setText(text);
    buffer.resetPerfStats();

    // Position at end of document
    buffer.setCaret({buffer.lineCount() - 1,
                     buffer.lineSpan(buffer.lineCount() - 1).length});

    // Count iterations instead of calling getText() each time (which creates a
    // copy)
    std::size_t remaining = DOC_SIZE;
    bench::Timer timer;
    while (remaining > 0) {
        buffer.backspace();
        --remaining;
    }
    double elapsed = timer.elapsedMs();

    auto stats = buffer.perfStats();

    std::printf("\n=== Sequential Backspace Benchmark ===\n");
    std::printf("  Characters deleted: %zu\n", DOC_SIZE);
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Per-delete: %.3f us\n", (elapsed * 1000.0) / DOC_SIZE);
    std::printf("  Total deletes tracked: %zu\n", stats.total_deletes);

    REQUIRE(buffer.getText().empty());
    REQUIRE(elapsed < 200.0);  // Less than 200ms
}

// ============================================================================
// LAYOUT BENCHMARKS
// ============================================================================

TEST_CASE("Benchmark: Line wrapping layout", "[benchmark][layout]") {
    const std::size_t DOC_SIZE = 50000;
    const std::size_t WRAP_WIDTH = 80;

    std::string text = bench::generateText(DOC_SIZE);
    TextBuffer buffer;
    buffer.setText(text);

    // Benchmark AoS (allocates strings)
    bench::Timer aosTimer;
    auto wrappedAoS = layoutWrappedLines(buffer, WRAP_WIDTH);
    double aosElapsed = aosTimer.elapsedMs();

    // Benchmark SoA (no string allocations)
    bench::Timer soaTimer;
    auto wrappedSoA = layoutWrappedLinesSoA(buffer, WRAP_WIDTH);
    double soaElapsed = soaTimer.elapsedMs();

    std::printf("\n=== Line Wrap Layout Benchmark ===\n");
    std::printf("  Document size: %zu chars\n", DOC_SIZE);
    std::printf("  Original lines: %zu\n", buffer.lineCount());
    std::printf("  Wrapped lines (at %zu): %zu\n", WRAP_WIDTH,
                wrappedAoS.size());
    std::printf("  AoS (with strings): %.3f ms (%.3f us/line)\n", aosElapsed,
                (aosElapsed * 1000.0) / buffer.lineCount());
    std::printf("  SoA (offsets only): %.3f ms (%.3f us/line)\n", soaElapsed,
                (soaElapsed * 1000.0) / buffer.lineCount());
    std::printf("  Speedup: %.2fx\n", aosElapsed / soaElapsed);

    REQUIRE(wrappedAoS.size() == wrappedSoA.size());
    REQUIRE(aosElapsed < 100.0);  // Less than 100ms
}

// ============================================================================
// BULK OPERATIONS
// ============================================================================

TEST_CASE("Benchmark: setText large document", "[benchmark][bulk]") {
    const std::size_t DOC_SIZE = 100000;

    std::string text = bench::generateText(DOC_SIZE);
    TextBuffer buffer;

    bench::Timer timer;
    buffer.setText(text);
    double elapsed = timer.elapsedMs();

    std::printf("\n=== setText Large Document Benchmark ===\n");
    std::printf("  Document size: %zu chars\n", DOC_SIZE);
    std::printf("  Lines created: %zu\n", buffer.lineCount());
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Throughput: %.2f MB/s\n",
                (DOC_SIZE / 1e6) / (elapsed / 1000.0));

    REQUIRE(buffer.getText().size() == DOC_SIZE);
    REQUIRE(elapsed < 20.0);  // Less than 20ms for 100k chars
}

TEST_CASE("Benchmark: getText large document", "[benchmark][bulk]") {
    const std::size_t DOC_SIZE = 100000;

    std::string text = bench::generateText(DOC_SIZE);
    TextBuffer buffer;
    buffer.setText(text);

    bench::Timer timer;
    std::string retrieved = buffer.getText();
    double elapsed = timer.elapsedMs();

    std::printf("\n=== getText Large Document Benchmark ===\n");
    std::printf("  Document size: %zu chars\n", DOC_SIZE);
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Throughput: %.2f MB/s\n",
                (DOC_SIZE / 1e6) / (elapsed / 1000.0));

    REQUIRE(retrieved.size() == DOC_SIZE);
    REQUIRE(elapsed < 10.0);  // Less than 10ms
}

// ============================================================================
// TYPING BURST SIMULATION
// ============================================================================

TEST_CASE("Benchmark: Typing burst simulation", "[benchmark][realistic]") {
    const std::size_t INITIAL_SIZE = 5000;
    const std::size_t BURST_SIZE = 500;  // Simulating fast typing

    std::string initial = bench::generateText(INITIAL_SIZE);
    TextBuffer buffer;
    buffer.setText(initial);

    // Position caret at middle of document
    std::size_t mid_row = buffer.lineCount() / 2;
    buffer.setCaret({mid_row, buffer.lineSpan(mid_row).length / 2});
    buffer.resetPerfStats();

    bench::Timer timer;
    for (std::size_t i = 0; i < BURST_SIZE; ++i) {
        buffer.insertChar("The quick brown fox jumps."[i % 26]);
        if ((i % 50) == 49) {
            buffer.insertChar('\n');
        }
    }
    double elapsed = timer.elapsedMs();

    auto stats = buffer.perfStats();

    std::printf("\n=== Typing Burst Simulation ===\n");
    std::printf("  Initial doc: %zu chars\n", INITIAL_SIZE);
    std::printf("  Chars typed: %zu\n", BURST_SIZE);
    std::printf("  Total time: %.3f ms\n", elapsed);
    std::printf("  Per-keystroke: %.3f us (%.1f chars/sec capability)\n",
                (elapsed * 1000.0) / BURST_SIZE,
                BURST_SIZE / (elapsed / 1000.0));
    std::printf("  Total inserts: %zu\n", stats.total_inserts);

    // 60 WPM = 5 chars/sec, so we need at least that
    double chars_per_sec = BURST_SIZE / (elapsed / 1000.0);
    REQUIRE(chars_per_sec > 1000);  // At least 1000 chars/sec capability
}

// ============================================================================
// SOA PERFORMANCE METRICS
// ============================================================================

TEST_CASE("Benchmark: SoA line access vs string copy", "[benchmark][soa]") {
    const std::size_t DOC_SIZE = 50000;
    const std::size_t ACCESS_COUNT = 10000;

    std::string text = bench::generateText(DOC_SIZE);
    TextBuffer buffer;
    buffer.setText(text);

    std::mt19937 rng(456);
    std::uniform_int_distribution<std::size_t> row_dist(0,
                                                        buffer.lineCount() - 1);

    // Test lineSpan (SoA, no allocation)
    bench::Timer spanTimer;
    std::size_t total_length = 0;
    for (std::size_t i = 0; i < ACCESS_COUNT; ++i) {
        std::size_t row = row_dist(rng);
        LineSpan span = buffer.lineSpan(row);
        total_length += span.length;
    }
    double spanElapsed = spanTimer.elapsedMs();

    // Reset RNG for fair comparison
    rng.seed(456);

    // Test lineString (allocates string)
    bench::Timer stringTimer;
    std::size_t total_length2 = 0;
    for (std::size_t i = 0; i < ACCESS_COUNT; ++i) {
        std::size_t row = row_dist(rng);
        std::string line = buffer.lineString(row);
        total_length2 += line.size();
    }
    double stringElapsed = stringTimer.elapsedMs();

    std::printf("\n=== SoA Line Access Benchmark ===\n");
    std::printf("  Document: %zu chars, %zu lines\n", DOC_SIZE,
                buffer.lineCount());
    std::printf("  Random accesses: %zu\n", ACCESS_COUNT);
    std::printf("  lineSpan (SoA): %.3f ms (%.3f us/access)\n", spanElapsed,
                (spanElapsed * 1000.0) / ACCESS_COUNT);
    std::printf("  lineString (copy): %.3f ms (%.3f us/access)\n",
                stringElapsed, (stringElapsed * 1000.0) / ACCESS_COUNT);
    std::printf("  Speedup: %.2fx\n", stringElapsed / spanElapsed);

    REQUIRE(total_length == total_length2);
    // SoA should generally be faster; allow 20% tolerance for system noise
    REQUIRE(spanElapsed < stringElapsed * 1.2);
}

// ============================================================================
// 100K-WORD DOCUMENT BENCHMARKS (unit-level; no GUI, no rendering)
// ============================================================================

TEST_CASE("Benchmark: 100K-word document load", "[benchmark][large-doc]") {
    const std::string text = bench::generateWordDocument();

    // Sanity: generator contract — exactly 100,000 words, 5,000 lines.
    {
        TextBuffer probe;
        probe.setText(text);
        REQUIRE(probe.stats().words == 100000);
        REQUIRE(probe.lineCount() == 5000);
        // NOTE: deliberately not REQUIRE(getText() == text) — under
        // `make bench-unit` (--success) Catch2 would print the whole
        // 543 KB string. Size + word/line counts pin the content here;
        // full equality is covered by small-doc tests elsewhere.
        REQUIRE(probe.textSize() == text.size());
    }

    // (a1) In-memory load: setText only.
    std::size_t loadedLines = 0;
    auto setTextMedian = bench::medianOf3Runs([&] {
        TextBuffer buffer;
        bench::Timer timer;
        buffer.setText(text);
        double elapsed = timer.elapsedMs();
        loadedLines = buffer.lineCount();
        return elapsed;
    });

    // (a2) File load path: write once (untimed), then time the real
    // loadTextFileEx path (file I/O + setText) on the .txt plain-text
    // branch, matching what Open does for a .txt file.
    const auto tmpPath =
        std::filesystem::temp_directory_path() / "wordproc_100k_bench.txt";
    {
        std::ofstream ofs(tmpPath, std::ios::binary);
        REQUIRE(ofs.good());
        ofs << text;
    }
    std::size_t fileLoadedLines = 0;
    auto fileMedian = bench::medianOf3Runs([&] {
        TextBuffer buffer;
        bench::Timer timer;
        auto result = loadTextFileEx(buffer, tmpPath.string());
        double elapsed = timer.elapsedMs();
        REQUIRE(result.success);
        fileLoadedLines = buffer.lineCount();
        return elapsed;
    });
    std::filesystem::remove(tmpPath);

    std::printf("\n=== 100K-Word Load Benchmark ===\n");
    std::printf("  Document: %zu chars, 5,000 lines, 100,000 words\n",
                text.size());
    std::printf("  setText: %.3f ms (%.2f MB/s)\n", setTextMedian.median_ms,
                (text.size() / 1e6) / (setTextMedian.median_ms / 1000.0));
    bench::printMedianRuns("setText runs", setTextMedian);
    std::printf("  loadTextFileEx (.txt): %.3f ms (%.2f MB/s)\n",
                fileMedian.median_ms,
                (text.size() / 1e6) / (fileMedian.median_ms / 1000.0));
    bench::printMedianRuns("file-load runs", fileMedian);

    REQUIRE(loadedLines == 5000);
    REQUIRE(fileLoadedLines == 5000);
    REQUIRE(setTextMedian.median_ms < 100.0);
    REQUIRE(fileMedian.median_ms < 200.0);
}

TEST_CASE("Benchmark: 100K-word full layout/wrap",
            "[benchmark][large-doc]") {
    const std::size_t WRAP_WIDTH = 80;
    const std::string text = bench::generateWordDocument();
    TextBuffer buffer;
    buffer.setText(text);

    // SoA is the layout used at scale; AoS is the comparison (it copies
    // every line's string). Both are timed as median of 3.
    std::size_t wrappedCount = 0;
    auto soaMedian = bench::medianOf3Runs([&] {
        bench::Timer timer;
        auto wrapped = layoutWrappedLinesSoA(buffer, WRAP_WIDTH);
        double elapsed = timer.elapsedMs();
        wrappedCount = wrapped.size();
        return elapsed;
    });
    std::size_t aosWrappedCount = 0;
    auto aosMedian = bench::medianOf3Runs([&] {
        bench::Timer timer;
        auto wrapped = layoutWrappedLines(buffer, WRAP_WIDTH);
        double elapsed = timer.elapsedMs();
        aosWrappedCount = wrapped.size();
        return elapsed;
    });

    std::printf("\n=== 100K-Word Full Layout Benchmark ===\n");
    std::printf("  Document: %zu chars, %zu lines, wrap width %zu\n",
                text.size(), buffer.lineCount(), WRAP_WIDTH);
    std::printf("  Wrapped lines: %zu\n", wrappedCount);
    bench::printMedianRuns("SoA layout", soaMedian);
    bench::printMedianRuns("AoS layout", aosMedian);

    REQUIRE(wrappedCount == aosWrappedCount);
    REQUIRE(wrappedCount > buffer.lineCount());
    REQUIRE(soaMedian.median_ms < 100.0);
}

TEST_CASE("Benchmark: 100K-word mid-document edit latency",
            "[benchmark][large-doc]") {
    const std::string text = bench::generateWordDocument();

    // One insert + one backspace at the middle of the doc, on a fresh
    // buffer per run: setText leaves the gap at the end, so the insert
    // pays the full gap move to the middle — the cost of a keystroke
    // after jumping to the middle — and the backspace deletes it, so
    // the doc is unchanged afterwards. Setup (setText + setCaret) is
    // outside the timed region.
    auto editMedian = bench::medianOf3Runs([&] {
        TextBuffer buffer;
        buffer.setText(text);
        const std::size_t midRow = buffer.lineCount() / 2;
        const std::size_t midCol =
            buffer.lineSpan(midRow).length / 2;
        buffer.setCaret({midRow, midCol});

        bench::Timer timer;
        buffer.insertChar('X');
        buffer.backspace();
        double elapsed = timer.elapsedMs();

        // Same no-huge-expansion rule as above: size + line count.
        REQUIRE(buffer.textSize() == text.size());
        REQUIRE(buffer.lineCount() == 5000);
        return elapsed;
    });

    std::printf("\n=== 100K-Word Mid-Document Edit Benchmark ===\n");
    std::printf("  Operation: insertChar + backspace at line %zu (middle)\n",
                static_cast<std::size_t>(5000) / 2);
    std::printf("  Median: %.3f us/pair (runs %.3f, %.3f, %.3f us)\n",
                editMedian.median_ms * 1000.0,
                editMedian.runs_ms[0] * 1000.0,
                editMedian.runs_ms[1] * 1000.0,
                editMedian.runs_ms[2] * 1000.0);

    // A keystroke must stay far under the 16 ms frame budget.
    REQUIRE(editMedian.median_ms < 10.0);
}

TEST_CASE("Benchmark: 100K-word viewport-window scroll proxy",
            "[benchmark][large-doc]") {
    // PROXY, named honestly: there is no unit-level scrolling to measure.
    // RenderCache::rebuild always starts at row 0 (scrolling is a TODO),
    // and layoutWrappedLinesSoA takes no range, so no API lays out one
    // viewport. This proxy walks successive 50-source-line windows over
    // the whole doc and applies the same per-line wrap arithmetic the
    // SoA layout uses (ceil(len / width), 1 for an empty line), without
    // allocating a LayoutResult. It estimates what windowed layout
    // would cost if layout were windowed; it is NOT a measured scroll.
    // A real scroll benchmark still needs the GUI binary.
    const std::size_t WRAP_WIDTH = 80;
    const std::size_t WINDOW_LINES = 50;
    const std::string text = bench::generateWordDocument();
    TextBuffer buffer;
    buffer.setText(text);
    const LayoutResult full = layoutWrappedLinesSoA(buffer, WRAP_WIDTH);
    const std::size_t numWindows =
        (buffer.lineCount() + WINDOW_LINES - 1) / WINDOW_LINES;

    std::size_t proxyWrapped = 0;
    auto proxyMedian = bench::medianOf3Runs([&] {
        bench::Timer timer;
        std::size_t totalWrapped = 0;
        std::size_t totalChars = 0;  // Sink: keeps the loop from being
                                     // optimized away; verified below.
        for (std::size_t win = 0; win < numWindows; ++win) {
            const std::size_t first = win * WINDOW_LINES;
            const std::size_t last =
                std::min(first + WINDOW_LINES, buffer.lineCount());
            for (std::size_t row = first; row < last; ++row) {
                const LineSpan span = buffer.lineSpan(row);
                totalWrapped += span.length == 0
                                    ? 1
                                    : (span.length + WRAP_WIDTH - 1) /
                                          WRAP_WIDTH;
                totalChars += span.length;
            }
        }
        double elapsed = timer.elapsedMs();
        proxyWrapped = totalWrapped;
        REQUIRE(totalChars == text.size() - (buffer.lineCount() - 1));
        return elapsed;
    });

    std::printf("\n=== 100K-Word Viewport-Window Scroll PROXY ===\n");
    std::printf("  PROXY: windowed wrap-count, NOT real scrolling\n");
    std::printf("  Windows: %zu x %zu source lines, wrap width %zu\n",
                numWindows, WINDOW_LINES, WRAP_WIDTH);
    std::printf("  Wrapped lines (proxy): %zu (full layout: %zu)\n",
                proxyWrapped, full.size());
    bench::printMedianRuns("proxy total", proxyMedian);
    std::printf("  Per-window: %.3f us (proxy total / %zu windows)\n",
                (proxyMedian.median_ms * 1000.0) /
                    static_cast<double>(numWindows),
                numWindows);

    // The proxy must agree with the full layout — otherwise it is not
    // measuring the same wrapping.
    REQUIRE(proxyWrapped == full.size());
    REQUIRE(proxyMedian.median_ms < 100.0);
}
