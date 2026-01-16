#include "catch2/catch.hpp"
#include "../src/editor/document_io.h"
#include <filesystem>
#include <fstream>

namespace {
    // Helper to create a temp directory for tests
    std::filesystem::path test_dir() {
        auto dir = std::filesystem::temp_directory_path() / "wordproc_test";
        std::filesystem::create_directories(dir);
        return dir;
    }

    // Helper to clean up after tests
    struct TestDirGuard {
        std::filesystem::path dir;
        TestDirGuard() : dir(test_dir()) {}
        ~TestDirGuard() {
            std::error_code ec;
            std::filesystem::remove_all(dir, ec);
        }
    };
}

TEST_CASE("saveTextFile and loadTextFile roundtrip", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "test_doc.wpdoc").string();

    SECTION("save and load plain text") {
        TextBuffer original;
        original.setText("Hello, World!");
        
        REQUIRE(saveTextFile(original, path));
        
        TextBuffer loaded;
        REQUIRE(loadTextFile(loaded, path));
        REQUIRE(loaded.getText() == "Hello, World!");
    }

    SECTION("save and load multiline text") {
        TextBuffer original;
        original.setText("Line 1\nLine 2\nLine 3");
        
        REQUIRE(saveTextFile(original, path));
        
        TextBuffer loaded;
        REQUIRE(loadTextFile(loaded, path));
        REQUIRE(loaded.getText() == "Line 1\nLine 2\nLine 3");
    }

    SECTION("save and load preserves style metadata") {
        TextBuffer original;
        original.setText("Styled text");
        TextStyle style;
        style.bold = true;
        style.italic = true;
        style.font = "TestFont";
        original.setTextStyle(style);
        
        REQUIRE(saveTextFile(original, path));
        
        TextBuffer loaded;
        REQUIRE(loadTextFile(loaded, path));
        
        TextStyle loaded_style = loaded.textStyle();
        REQUIRE(loaded_style.bold == true);
        REQUIRE(loaded_style.italic == true);
        REQUIRE(loaded_style.font == "TestFont");
    }
}

TEST_CASE("loadTextFile handles plain text files", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "plain.txt").string();
    
    SECTION("loads plain text as-is when not JSON") {
        {
            std::ofstream ofs(path);
            ofs << "Just plain text";
        }
        
        TextBuffer buffer;
        REQUIRE(loadTextFile(buffer, path));
        REQUIRE(buffer.getText() == "Just plain text");
    }
}

TEST_CASE("saveTextFile creates parent directories", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "subdir" / "nested" / "doc.wpdoc").string();
    
    TextBuffer buffer;
    buffer.setText("Nested save");
    
    REQUIRE(saveTextFile(buffer, path));
    REQUIRE(std::filesystem::exists(path));
}

TEST_CASE("loadTextFile returns false for missing file", "[document_io]") {
    TextBuffer buffer;
    REQUIRE_FALSE(loadTextFile(buffer, "/nonexistent/path/file.txt"));
}
