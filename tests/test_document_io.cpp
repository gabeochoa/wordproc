#include <filesystem>
#include <fstream>

#include "../src/editor/document_io.h"
#include "catch2/catch.hpp"

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
}  // namespace

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
    
    SECTION("save and load preserves underline and strikethrough") {
        TextBuffer original;
        original.setText("Emphasized text");
        TextStyle style;
        style.underline = true;
        style.strikethrough = true;
        original.setTextStyle(style);

        REQUIRE(saveTextFile(original, path));

        TextBuffer loaded;
        REQUIRE(loadTextFile(loaded, path));

        TextStyle loaded_style = loaded.textStyle();
        REQUIRE(loaded_style.underline == true);
        REQUIRE(loaded_style.strikethrough == true);
    }
    
    SECTION("save and load preserves text color and highlight color") {
        TextBuffer original;
        original.setText("Colored text");
        TextStyle style;
        style.textColor = TextColors::Blue;
        style.highlightColor = HighlightColors::Yellow;
        original.setTextStyle(style);

        REQUIRE(saveTextFile(original, path));

        TextBuffer loaded;
        REQUIRE(loadTextFile(loaded, path));

        TextStyle loaded_style = loaded.textStyle();
        REQUIRE(loaded_style.textColor == TextColors::Blue);
        REQUIRE(loaded_style.highlightColor == HighlightColors::Yellow);
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

TEST_CASE("saveDocumentEx and loadDocumentEx roundtrip with full settings", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "full_doc.wpdoc").string();

    SECTION("save and load preserves page settings") {
        TextBuffer original;
        original.setText("Page layout test");
        
        DocumentSettings settings;
        settings.pageSettings.mode = PageMode::Paged;
        settings.pageSettings.pageWidth = 800.0f;
        settings.pageSettings.pageHeight = 1000.0f;
        settings.pageSettings.pageMargin = 50.0f;
        settings.pageSettings.lineWidthLimit = 100.0f;
        settings.textStyle.bold = true;
        settings.textStyle.font = "Arial";
        settings.textStyle.fontSize = 14;
        original.setTextStyle(settings.textStyle);

        auto saveResult = saveDocumentEx(original, settings, path);
        REQUIRE(saveResult.success);

        TextBuffer loaded;
        DocumentSettings loadedSettings;
        auto loadResult = loadDocumentEx(loaded, loadedSettings, path);
        REQUIRE(loadResult.success);
        REQUIRE(loaded.getText() == "Page layout test");
        
        // Verify page settings
        REQUIRE(loadedSettings.pageSettings.mode == PageMode::Paged);
        REQUIRE(loadedSettings.pageSettings.pageWidth == Approx(800.0f));
        REQUIRE(loadedSettings.pageSettings.pageHeight == Approx(1000.0f));
        REQUIRE(loadedSettings.pageSettings.pageMargin == Approx(50.0f));
        REQUIRE(loadedSettings.pageSettings.lineWidthLimit == Approx(100.0f));
        
        // Verify text style
        REQUIRE(loadedSettings.textStyle.bold == true);
        REQUIRE(loadedSettings.textStyle.font == "Arial");
        REQUIRE(loadedSettings.textStyle.fontSize == 14);
    }
    
    SECTION("pageless mode is preserved") {
        TextBuffer original;
        original.setText("Pageless test");
        
        DocumentSettings settings;
        settings.pageSettings.mode = PageMode::Pageless;
        settings.pageSettings.lineWidthLimit = 80.0f;

        auto saveResult = saveDocumentEx(original, settings, path);
        REQUIRE(saveResult.success);

        TextBuffer loaded;
        DocumentSettings loadedSettings;
        auto loadResult = loadDocumentEx(loaded, loadedSettings, path);
        REQUIRE(loadResult.success);
        
        REQUIRE(loadedSettings.pageSettings.mode == PageMode::Pageless);
        REQUIRE(loadedSettings.pageSettings.lineWidthLimit == Approx(80.0f));
    }
    
    SECTION("default page settings are used when loading old document format") {
        // Simulate old format without pageLayout field
        {
            std::ofstream ofs(path);
            ofs << R"({"version":1,"text":"Old doc","style":{"bold":false,"italic":false,"font":"Default","fontSize":16}})";
        }
        
        TextBuffer loaded;
        DocumentSettings loadedSettings;
        auto loadResult = loadDocumentEx(loaded, loadedSettings, path);
        REQUIRE(loadResult.success);
        REQUIRE(loaded.getText() == "Old doc");
        
        // Default page settings should be used
        REQUIRE(loadedSettings.pageSettings.mode == PageMode::Pageless);
        REQUIRE(loadedSettings.pageSettings.pageWidth == Approx(612.0f)); // Default letter size
    }
}

TEST_CASE("DocumentSettings separation from app settings", "[document_io]") {
    // This test verifies the concept: document settings are per-document,
    // not global app settings. Each document maintains its own settings.
    
    TestDirGuard guard;
    std::string path1 = (guard.dir / "doc1.wpdoc").string();
    std::string path2 = (guard.dir / "doc2.wpdoc").string();
    
    // Create two documents with different settings
    TextBuffer buf1, buf2;
    buf1.setText("Document 1");
    buf2.setText("Document 2");
    
    DocumentSettings settings1, settings2;
    settings1.pageSettings.mode = PageMode::Paged;
    settings1.textStyle.fontSize = 12;
    
    settings2.pageSettings.mode = PageMode::Pageless;
    settings2.textStyle.fontSize = 18;
    
    REQUIRE(saveDocumentEx(buf1, settings1, path1).success);
    REQUIRE(saveDocumentEx(buf2, settings2, path2).success);
    
    // Load and verify each document maintains its own settings
    TextBuffer loaded1, loaded2;
    DocumentSettings loadedSettings1, loadedSettings2;
    
    REQUIRE(loadDocumentEx(loaded1, loadedSettings1, path1).success);
    REQUIRE(loadDocumentEx(loaded2, loadedSettings2, path2).success);
    
    REQUIRE(loadedSettings1.pageSettings.mode == PageMode::Paged);
    REQUIRE(loadedSettings1.textStyle.fontSize == 12);
    
    REQUIRE(loadedSettings2.pageSettings.mode == PageMode::Pageless);
    REQUIRE(loadedSettings2.textStyle.fontSize == 18);
}

<<<<<<< HEAD
TEST_CASE("Options flags persist with the document", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "options_doc.wpdoc").string();

    SECTION("text boundaries + spell-as-you-type round-trip") {
        TextBuffer original;
        original.setText("Options test");
        DocumentSettings settings;
        settings.showTextBoundaries = true;
        settings.spellCheckAsYouType = false;
        REQUIRE(saveDocumentEx(original, settings, path).success);

        TextBuffer loaded;
        DocumentSettings loadedSettings;
        REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
        REQUIRE(loadedSettings.showTextBoundaries);
        REQUIRE_FALSE(loadedSettings.spellCheckAsYouType);
    }
    SECTION("old files without the options field load defaults") {
        {
            std::ofstream ofs(path);
            ofs << R"({"version":1,"text":"Old doc"})";
        }
        TextBuffer loaded;
        DocumentSettings loadedSettings;
        REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
        REQUIRE_FALSE(loadedSettings.showTextBoundaries);
        REQUIRE(loadedSettings.spellCheckAsYouType);
    }
}

TEST_CASE("Text boundaries are off by default", "[document_io]") {
    // Word 95 shows margin boundary guides only with Tools >
    // Options > View "Text boundaries" enabled. There is no
    // settings UI for it this round; the flag exists to gate
    // drawPageBackground's boundary lines.
    DocumentSettings settings;
    REQUIRE_FALSE(settings.showTextBoundaries);
    settings.showTextBoundaries = true;
    REQUIRE(settings.showTextBoundaries);
    DocumentSettings fresh;
    REQUIRE_FALSE(fresh.showTextBoundaries);
}

TEST_CASE("Header/footer text + config round-trip (Batch 9)", "[document_io]") {
    TestDirGuard guard;
    std::string path = (guard.dir / "header_footer.wpdoc").string();

    SECTION("header/footer text and page-number config persist") {
        TextBuffer body;
        body.setText("Body text");
        TextBuffer header;
        header.setText("My Header");
        TextBuffer footer;
        footer.setText("My Footer");
        DocumentSettings settings;
        settings.footer.center.showPageNumber = true;
        settings.footer.center.format = PageNumberFormat::Arabic;
        settings.header.differentFirstPage = true;
        TableList tables;
        REQUIRE(saveDocumentWithTables(body, settings, tables, path,
                                       &header, &footer)
                    .success);

        TextBuffer loadedBody;
        TextBuffer loadedHeader;
        TextBuffer loadedFooter;
        DocumentSettings loadedSettings;
        TableList loadedTables;
        REQUIRE(loadDocumentWithTables(loadedBody, loadedSettings,
                                       loadedTables, path, &loadedHeader,
                                       &loadedFooter)
                    .success);
        REQUIRE(loadedBody.getText() == "Body text");
        REQUIRE(loadedHeader.getText() == "My Header");
        REQUIRE(loadedFooter.getText() == "My Footer");
        REQUIRE(loadedSettings.footer.center.showPageNumber);
        REQUIRE(loadedSettings.footer.center.format ==
                PageNumberFormat::Arabic);
        REQUIRE(loadedSettings.header.differentFirstPage);
    }

    SECTION("loading a header-less file clears stale header text") {
        TextBuffer body;
        body.setText("Plain body");
        DocumentSettings settings;
        TableList tables;
        REQUIRE(saveDocumentWithTables(body, settings, tables, path)
                    .success);

        TextBuffer loadedBody;
        TextBuffer loadedHeader;
        loadedHeader.setText("Stale header");
        TextBuffer loadedFooter;
        loadedFooter.setText("Stale footer");
        DocumentSettings loadedSettings;
        TableList loadedTables;
        REQUIRE(loadDocumentWithTables(loadedBody, loadedSettings,
                                       loadedTables, path, &loadedHeader,
                                       &loadedFooter)
                    .success);
        REQUIRE(loadedHeader.getText().empty());
        REQUIRE(loadedFooter.getText().empty());
    }

    SECTION("header/footer config persists through saveDocumentEx") {
        TextBuffer body;
        body.setText("Config only");
        DocumentSettings settings;
        settings.footer.center.showPageNumber = true;
        settings.footer.center.showTotalPages = true;
        settings.header.differentFirstPage = true;
        REQUIRE(saveDocumentEx(body, settings, path).success);

        TextBuffer loaded;
        DocumentSettings loadedSettings;
        REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
        REQUIRE(loadedSettings.footer.center.showPageNumber);
        REQUIRE(loadedSettings.footer.center.showTotalPages);
        REQUIRE(loadedSettings.header.differentFirstPage);
    }
=======
TEST_CASE("failed saves preserve the previous document", "[document_io]") {
    TestDirGuard guard;
    const auto path = guard.dir / "preserved.wpdoc";
    TextBuffer before;
    before.setText("Original document");
    REQUIRE(saveTextFile(before, path.string()));
    std::filesystem::create_directory(path.string() + ".afh-tmp");
    TextBuffer changed;
    changed.setText("Unsaved replacement");
    REQUIRE_FALSE(saveTextFile(changed, path.string()));
    TextBuffer loaded;
    REQUIRE(loadTextFile(loaded, path.string()));
    REQUIRE(loaded.getText() == "Original document");
}

TEST_CASE("unusable document parent reports failure", "[document_io]") {
    TestDirGuard guard;
    const auto parent = guard.dir / "file_not_directory";
    { std::ofstream out(parent); out << "existing"; }
    TextBuffer buffer;
    buffer.setText("Unsaved document");
    REQUIRE_FALSE(saveTextFile(buffer, (parent / "document.wpdoc").string()));
    std::ifstream in(parent);
    std::string contents;
    in >> contents;
    REQUIRE(contents == "existing");
>>>>>>> 0e46e6d (Gabe: zig build + local WIP (from his Mac, base af741be))
}
