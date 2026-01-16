#include <filesystem>
#include <fstream>

#include "../src/editor/document_io.h"
#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

// Helper to create test files
void writeTestFile(const std::string& path, const std::string& content) {
    std::ofstream ofs(path);
    ofs << content;
}

TEST_CASE("Format validator: malformed JSON", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_malformed.wpdoc";

    writeTestFile(path, R"({
        "version": 1,
        "text": "Missing closing quote
    })");

    auto result = loadTextFileEx(buffer, path);

    REQUIRE(result.success);              // Still loads
    REQUIRE(result.usedFallback);         // Falls back to plain text
    REQUIRE_FALSE(result.error.empty());  // Has error message

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: truncated file", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_truncated.wpdoc";

    writeTestFile(path, R"({"version": 1, "text": "incomplete)");

    auto result = loadTextFileEx(buffer, path);

    REQUIRE(result.success);
    REQUIRE(result.usedFallback);

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: wrong version", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_wrong_version.wpdoc";

    writeTestFile(path, R"({
        "version": 999,
        "text": "Future version",
        "style": {"bold": false}
    })");

    auto result = loadTextFileEx(buffer, path);

    // Should still load but with warning
    REQUIRE(result.success);
    REQUIRE(result.usedFallback);  // Version mismatch triggers fallback flag
    REQUIRE(buffer.getText() == "Future version");

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: missing text field", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_missing_text.wpdoc";

    writeTestFile(path, R"({
        "version": 1,
        "style": {"bold": true}
    })");

    auto result = loadTextFileEx(buffer, path);

    REQUIRE(result.success);
    REQUIRE(result.usedFallback);  // Missing text field triggers fallback

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: valid minimal file", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_valid.wpdoc";

    writeTestFile(path, R"({
        "version": 1,
        "text": "Hello, World!"
    })");

    auto result = loadTextFileEx(buffer, path);

    REQUIRE(result.success);
    REQUIRE_FALSE(result.usedFallback);
    REQUIRE(buffer.getText() == "Hello, World!");

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: font size clamping", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_fontsize.wpdoc";

    SECTION("oversized font clamped to 72") {
        writeTestFile(path, R"({
            "version": 1,
            "text": "Test",
            "style": {"fontSize": 200}
        })");

        auto result = loadTextFileEx(buffer, path);
        REQUIRE(result.success);
        REQUIRE(buffer.textStyle().fontSize == 72);
    }

    SECTION("undersized font clamped to 8") {
        writeTestFile(path, R"({
            "version": 1,
            "text": "Test",
            "style": {"fontSize": 2}
        })");

        auto result = loadTextFileEx(buffer, path);
        REQUIRE(result.success);
        REQUIRE(buffer.textStyle().fontSize == 8);
    }

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: plain text import", "[validator]") {
    TextBuffer buffer;
    std::string path = "output/test_plain.txt";

    writeTestFile(path, "Just plain text\nWith multiple lines");

    auto result = loadTextFileEx(buffer, path);

    REQUIRE(result.success);
    REQUIRE(result.usedFallback);  // Plain text uses fallback
    REQUIRE(buffer.getText() == "Just plain text\nWith multiple lines");

    std::filesystem::remove(path);
}

TEST_CASE("Format validator: file not found", "[validator]") {
    TextBuffer buffer;

    auto result = loadTextFileEx(buffer, "nonexistent_file.wpdoc");

    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}
