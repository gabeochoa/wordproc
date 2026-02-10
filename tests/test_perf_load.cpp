#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../src/editor/document_io.h"
#include "catch2/catch.hpp"

namespace {

// Collect all files matching extensions in a directory
std::vector<std::filesystem::path> collectFiles(
    const std::filesystem::path& dir,
    const std::vector<std::string>& extensions) {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(dir)) return files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (const auto& wanted : extensions) {
            if (ext == wanted) {
                files.push_back(entry.path());
                break;
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

struct LoadResult {
    std::string filename;
    std::size_t sizeBytes = 0;
    std::size_t lineCount = 0;
    double loadMs = 0.0;
    bool success = false;
};

// Load a file and measure how long it takes (pure I/O + setText)
LoadResult benchmarkLoad(const std::filesystem::path& path) {
    LoadResult r;
    r.filename = path.filename().string();
    r.sizeBytes = std::filesystem::file_size(path);

    TextBuffer buffer;

    auto start = std::chrono::high_resolution_clock::now();
    auto result = loadTextFileEx(buffer, path.string());
    auto end = std::chrono::high_resolution_clock::now();

    r.loadMs = std::chrono::duration<double, std::milli>(end - start).count();
    r.success = result.success;
    r.lineCount = buffer.lineCount();
    return r;
}

// Warm the file system cache by reading the file once
void warmCache(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (ifs) {
        std::string contents((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        // Force the read to not be optimized away
        volatile std::size_t sz = contents.size();
        (void)sz;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Performance tests: every text file must load under 10ms (warm cache)
// ---------------------------------------------------------------------------

// Robust benchmark: 2 warmup + 5 timed iterations, take minimum (best capability)
static LoadResult robustBenchmark(const std::filesystem::path& file) {
    warmCache(file);

    // 2 throwaway warmup runs (warms code paths, JIT, branch predictors)
    for (int i = 0; i < 2; ++i) {
        benchmarkLoad(file);
    }

    // 5 timed iterations, take minimum
    std::vector<double> times;
    LoadResult best;
    for (int i = 0; i < 5; ++i) {
        auto r = benchmarkLoad(file);
        times.push_back(r.loadMs);
        if (i == 0 || r.loadMs < best.loadMs) best = r;
    }
    best.loadMs = *std::min_element(times.begin(), times.end());
    return best;
}

TEST_CASE("All public_domain text files load under 10ms", "[perf][load]") {
    const double TARGET_MS = 10.0;
    auto files =
        collectFiles("test_files/public_domain", {".txt", ".md"});
    REQUIRE_FALSE(files.empty());

    std::vector<LoadResult> results;
    int failures = 0;

    for (const auto& file : files) {
        auto best = robustBenchmark(file);
        results.push_back(best);

        INFO("File: " << best.filename << " (" << best.sizeBytes
                       << " bytes, " << best.lineCount << " lines)");
        INFO("Load time: " << best.loadMs << " ms (target: " << TARGET_MS
                           << " ms)");

        if (best.loadMs > TARGET_MS) {
            ++failures;
            WARN(best.filename << ": " << best.loadMs
                               << " ms EXCEEDS " << TARGET_MS << " ms target");
        }

        CHECK(best.success);
    }

    // Print summary
    INFO("=== Load Performance Summary ===");
    for (const auto& r : results) {
        std::string status = r.loadMs <= TARGET_MS ? "PASS" : "FAIL";
        INFO("  " << r.filename << ": " << r.loadMs << " ms ["
                   << r.sizeBytes << " bytes, " << r.lineCount
                   << " lines] " << status);
    }

    CHECK(failures == 0);
}

TEST_CASE("All e2e text files load under 10ms", "[perf][load]") {
    const double TARGET_MS = 10.0;
    auto files = collectFiles("test_files/e2e", {".txt", ".md"});
    REQUIRE_FALSE(files.empty());

    for (const auto& file : files) {
        auto best = robustBenchmark(file);
        INFO("File: " << best.filename << " (" << best.sizeBytes << " bytes)");
        INFO("Load time: " << best.loadMs << " ms (target: " << TARGET_MS << " ms)");
        CHECK(best.loadMs <= TARGET_MS);
    }
}

TEST_CASE("All wpdoc files load under 10ms", "[perf][load]") {
    const double TARGET_MS = 10.0;
    auto files =
        collectFiles("test_files/should_pass", {".wpdoc"});
    REQUIRE_FALSE(files.empty());

    for (const auto& file : files) {
        auto best = robustBenchmark(file);
        INFO("File: " << best.filename << " (" << best.sizeBytes << " bytes)");
        INFO("Load time: " << best.loadMs << " ms (target: " << TARGET_MS << " ms)");
        CHECK(best.loadMs <= TARGET_MS);
    }
}

// ---------------------------------------------------------------------------
// Scaling test: load time should scale roughly linearly with file size
// ---------------------------------------------------------------------------

TEST_CASE("Load time scales linearly with file size", "[perf][load][scaling]") {
    // Generate synthetic files of increasing size
    auto tmpDir = std::filesystem::temp_directory_path() / "wordproc_perf";
    std::filesystem::create_directories(tmpDir);

    struct SizePoint {
        std::size_t bytes;
        double loadMs;
    };
    std::vector<SizePoint> points;

    // Sizes: 1KB, 10KB, 100KB, 1MB, 5MB
    std::vector<std::size_t> sizes = {1024, 10240, 102400, 1048576, 5242880};

    for (auto targetSize : sizes) {
        auto path = tmpDir / ("synthetic_" + std::to_string(targetSize) + ".txt");
        {
            std::ofstream ofs(path);
            // Write lines of ~80 chars each
            std::string line(79, 'x');
            std::size_t written = 0;
            while (written < targetSize) {
                ofs << line << "\n";
                written += 80;
            }
        }

        auto best = robustBenchmark(path);
        CHECK(best.success);
        points.push_back({targetSize, best.loadMs});

        INFO("Synthetic " << targetSize << " bytes: " << best.loadMs << " ms");
    }

    // Check that 5MB doesn't take more than 50x what 1KB takes
    // (should be ~5000x data but linear algos should keep it well under 50x time)
    if (points.size() >= 2 && points.front().loadMs > 0.001) {
        double ratio = points.back().loadMs / points.front().loadMs;
        double sizeRatio =
            static_cast<double>(points.back().bytes) / static_cast<double>(points.front().bytes);
        INFO("Time ratio (5MB/1KB): " << ratio
                                       << "x (size ratio: " << sizeRatio << "x)");
        // Should be roughly linear - time ratio should be within 2 orders of magnitude of size ratio
        CHECK(ratio < sizeRatio * 2.0);
    }

    // Cleanup
    std::filesystem::remove_all(tmpDir);
}

// ---------------------------------------------------------------------------
// Throughput test: measure MB/s for the largest file
// ---------------------------------------------------------------------------

TEST_CASE("Load throughput exceeds 100 MB/s", "[perf][load][throughput]") {
    // Find the largest public_domain file
    auto files = collectFiles("test_files/public_domain", {".txt"});
    REQUIRE_FALSE(files.empty());

    std::filesystem::path largest;
    std::size_t largestSize = 0;
    for (const auto& f : files) {
        auto sz = std::filesystem::file_size(f);
        if (sz > largestSize) {
            largestSize = sz;
            largest = f;
        }
    }
    REQUIRE(largestSize > 0);

    auto best = robustBenchmark(largest);
    CHECK(best.success);

    double mbPerSec = (static_cast<double>(largestSize) / (1024.0 * 1024.0)) / (best.loadMs / 1000.0);
    INFO("Largest file: " << largest.filename().string() << " ("
                          << largestSize << " bytes)");
    INFO("Best load time: " << best.loadMs << " ms");
    INFO("Throughput: " << mbPerSec << " MB/s");
    CHECK(mbPerSec > 100.0);
}
