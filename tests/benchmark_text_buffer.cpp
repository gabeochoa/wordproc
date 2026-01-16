#include "catch2/catch.hpp"
#include "../src/editor/text_buffer.h"
#include "../src/editor/text_layout.h"
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>

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
  std::uniform_int_distribution<std::size_t> line_dist(20, avg_line_length * 2);
  
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
  std::uniform_int_distribution<std::size_t> row_dist(0, buffer.lineCount() - 1);
  
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
  buffer.setCaret({buffer.lineCount() - 1, buffer.lineSpan(buffer.lineCount() - 1).length});
  
  bench::Timer timer;
  while (buffer.getText().size() > 0) {
    buffer.backspace();
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
  
  bench::Timer timer;
  auto wrapped = wrapLines(buffer.lines(), WRAP_WIDTH);
  double elapsed = timer.elapsedMs();
  
  std::printf("\n=== Line Wrap Layout Benchmark ===\n");
  std::printf("  Document size: %zu chars\n", DOC_SIZE);
  std::printf("  Original lines: %zu\n", buffer.lineCount());
  std::printf("  Wrapped lines (at %zu): %zu\n", WRAP_WIDTH, wrapped.size());
  std::printf("  Total time: %.3f ms\n", elapsed);
  std::printf("  Per-original-line: %.3f us\n", (elapsed * 1000.0) / buffer.lineCount());
  
  REQUIRE(elapsed < 50.0);  // Less than 50ms
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
  std::printf("  Throughput: %.2f MB/s\n", (DOC_SIZE / 1e6) / (elapsed / 1000.0));
  
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
  std::printf("  Throughput: %.2f MB/s\n", (DOC_SIZE / 1e6) / (elapsed / 1000.0));
  
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
  std::uniform_int_distribution<std::size_t> row_dist(0, buffer.lineCount() - 1);
  
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
  std::printf("  Document: %zu chars, %zu lines\n", DOC_SIZE, buffer.lineCount());
  std::printf("  Random accesses: %zu\n", ACCESS_COUNT);
  std::printf("  lineSpan (SoA): %.3f ms (%.3f us/access)\n",
              spanElapsed, (spanElapsed * 1000.0) / ACCESS_COUNT);
  std::printf("  lineString (copy): %.3f ms (%.3f us/access)\n",
              stringElapsed, (stringElapsed * 1000.0) / ACCESS_COUNT);
  std::printf("  Speedup: %.2fx\n", stringElapsed / spanElapsed);
  
  REQUIRE(total_length == total_length2);
  REQUIRE(spanElapsed < stringElapsed);  // SoA should be faster
}
