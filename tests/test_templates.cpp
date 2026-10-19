#include <filesystem>
#include <fstream>

#include "../src/editor/templates.h"
#include "catch2/catch.hpp"

namespace {
std::filesystem::path test_dir(const std::string& name) {
    auto dir =
        std::filesystem::temp_directory_path() / "wordproc_templates_test" / name;
    std::filesystem::create_directories(dir);
    return dir;
}
}  // namespace

TEST_CASE("paragraphStyles survive a save/load roundtrip", "[templates]") {
    auto dir = test_dir("roundtrip");
    std::string path = (dir / "styled.wpdoc").string();

    TextBuffer original;
    original.setText("Title\nBody\nSection");
    original.setCaret({0, 0});
    original.setCurrentParagraphStyle(ParagraphStyle::Title);
    original.setCaret({2, 0});
    original.setCurrentParagraphStyle(ParagraphStyle::Heading1);

    DocumentSettings settings;
    REQUIRE(saveDocumentEx(original, settings, path).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    REQUIRE(loaded.getText() == "Title\nBody\nSection");
    REQUIRE(loaded.lineParagraphStyle(0) == ParagraphStyle::Title);
    REQUIRE(loaded.lineParagraphStyle(1) == ParagraphStyle::Normal);
    REQUIRE(loaded.lineParagraphStyle(2) == ParagraphStyle::Heading1);

    // Unknown style names parse as Normal; a short array leaves the
    // remaining lines Normal.
    REQUIRE(paragraphStyleFromName("Heading 1") == ParagraphStyle::Heading1);
    REQUIRE(paragraphStyleFromName("heading2") == ParagraphStyle::Heading2);
    REQUIRE(paragraphStyleFromName("H3") == ParagraphStyle::Heading3);
    REQUIRE(paragraphStyleFromName("nope") == ParagraphStyle::Normal);
}

TEST_CASE("listTemplateNames scans only .wpdoc stems, sorted", "[templates]") {
    auto dir = test_dir("listing");
    std::ofstream(dir / "Beta.wpdoc") << "{}";
    std::ofstream(dir / "Alpha.wpdoc") << "{}";
    std::ofstream(dir / "notes.txt") << "not a template";
    std::ofstream(dir / "Gamma.WPDOC") << "{}";

    auto names = templates::listTemplateNames(dir);
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "Alpha");
    REQUIRE(names[1] == "Beta");
    REQUIRE(names[2] == "Gamma");
    REQUIRE(templates::listTemplateNames(dir / "missing").empty());
}

TEST_CASE("adoptTemplate carries text, styles, and caret", "[templates]") {
    TextBuffer source;
    source.setText("MEMORANDUM\n\nSummary");
    source.setCaret({0, 0});
    source.setCurrentParagraphStyle(ParagraphStyle::Title);
    source.setCaret({2, 0});
    source.setCurrentParagraphStyle(ParagraphStyle::Heading1);

    TextBuffer target;
    target.setText("old document\nwith two lines");
    DocumentSettings settings;
    templates::adoptTemplate(target, source, settings);
    REQUIRE(target.getText() == "MEMORANDUM\n\nSummary");
    REQUIRE(target.lineParagraphStyle(0) == ParagraphStyle::Title);
    REQUIRE(target.lineParagraphStyle(2) == ParagraphStyle::Heading1);
    REQUIRE(target.caret().row == 0);
    REQUIRE_FALSE(target.hasSelection());
}

TEST_CASE("shipped Memo template loads with its styled heading", "[templates]") {
    // make test runs from the repo root, where resources/templates/
    // lives; templateDir() prefers the afterhours provider when it
    // is initialized and falls back to <cwd>/resources/templates.
    TextBuffer buffer;
    DocumentSettings settings;
    auto path = templates::templateDir() / "Memo.wpdoc";
    REQUIRE(std::filesystem::exists(path));
    REQUIRE(templates::loadTemplate(path, buffer, settings));
    REQUIRE(buffer.getText().find("MEMORANDUM") != std::string::npos);
    REQUIRE(buffer.lineParagraphStyle(0) == ParagraphStyle::Title);
    REQUIRE(buffer.lineParagraphStyle(7) == ParagraphStyle::Heading1);

    auto names = templates::listTemplateNames(templates::templateDir());
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "Letter");
    REQUIRE(names[1] == "Memo");
    REQUIRE(names[2] == "Report");
}
