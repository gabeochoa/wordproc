#include <filesystem>
#include <fstream>

#include "../src/editor/columns_math.h"
#include "../src/editor/document_io.h"
#include "../src/editor/document_settings.h"
#include "../src/editor/scroll_math.h"
#include "catch2/catch.hpp"

namespace {
// A .wpdoc in the pre-per-side-margin shape: only pageMargin.
// Mirrors what saveDocumentEx wrote before Page Setup honored
// per-side margins.
void writeLegacyPageSetupFile(const std::string& path) {
    std::ofstream ofs(path);
    ofs << R"({"version":1,"text":"legacy","pageLayout":{)"
           R"("mode":"paged","pageWidth":792,"pageHeight":612,)"
           R"("pageMargin":50,"lineWidthLimit":0,"columns":1,)"
           R"("columnSpacing":36}})";
}
}  // namespace

TEST_CASE("Page Setup paper dimensions", "[page_setup]") {
    SECTION("portrait dims are the preset dims") {
        float w = 0.0f, h = 0.0f;
        getPageDimensions(PageSize::Letter, w, h);
        REQUIRE(w == 612.0f);
        REQUIRE(h == 792.0f);
        getPageDimensions(PageSize::Legal, w, h);
        REQUIRE(w == 612.0f);
        REQUIRE(h == 1008.0f);
        getPageDimensions(PageSize::Tabloid, w, h);
        REQUIRE(w == 792.0f);
        REQUIRE(h == 1224.0f);
        getPageDimensions(PageSize::A4, w, h);
        REQUIRE(w == 595.0f);
        REQUIRE(h == 842.0f);
        getPageDimensions(PageSize::A5, w, h);
        REQUIRE(w == 420.0f);
        REQUIRE(h == 595.0f);
        getPageDimensions(PageSize::B5, w, h);
        REQUIRE(w == 499.0f);
        REQUIRE(h == 709.0f);
        getPageDimensions(PageSize::Executive, w, h);
        REQUIRE(w == 522.0f);
        REQUIRE(h == 756.0f);
    }
    SECTION("Letter landscape is 11 x 8.5 inches") {
        PageSettings ps;
        ps.applyPageSize(PageSize::Letter);
        REQUIRE(ps.pageWidth == 612.0f);
        REQUIRE(ps.pageHeight == 792.0f);
        ps.setOrientation(PageOrientation::Landscape);
        REQUIRE(ps.orientation == PageOrientation::Landscape);
        REQUIRE(ps.pageWidth == 792.0f);   // 11" * 72
        REQUIRE(ps.pageHeight == 612.0f);  // 8.5" * 72
        // At 72 ppi (displayScale 1) the page spans 11 inches wide.
        REQUIRE(ps.pageWidth / 72.0f == Approx(11.0f));
        REQUIRE(ps.pageHeight / 72.0f == Approx(8.5f));
    }
    SECTION("setOrientation is idempotent, toggle is not") {
        PageSettings ps;
        ps.applyPageSize(PageSize::Letter);
        ps.setOrientation(PageOrientation::Landscape);
        ps.setOrientation(PageOrientation::Landscape);
        REQUIRE(ps.pageWidth == 792.0f);
        REQUIRE(ps.pageHeight == 612.0f);
        ps.setOrientation(PageOrientation::Portrait);
        REQUIRE(ps.pageWidth == 612.0f);
        REQUIRE(ps.pageHeight == 792.0f);
    }
    SECTION("applyPageSize after an orientation change recomputes") {
        PageSettings ps;
        ps.setOrientation(PageOrientation::Landscape);
        ps.applyPageSize(PageSize::Legal);
        REQUIRE(ps.pageWidth == 1008.0f);  // 14" tall side first
        REQUIRE(ps.pageHeight == 612.0f);
        ps.applyPageSize(PageSize::A4);
        REQUIRE(ps.pageWidth == 842.0f);
        REQUIRE(ps.pageHeight == 595.0f);
    }
    SECTION("size/orientation ids round-trip") {
        REQUIRE(std::string(pageSizeId(PageSize::A4)) == "a4");
        REQUIRE(pageSizeFromId("legal") == PageSize::Legal);
        REQUIRE(pageSizeFromId("bogus") == PageSize::Letter);
        REQUIRE(std::string(pageOrientationId(PageOrientation::Landscape)) ==
                "landscape");
        REQUIRE(pageOrientationFromId("landscape") ==
                PageOrientation::Landscape);
        REQUIRE(pageOrientationFromId("bogus") == PageOrientation::Portrait);
    }
}

TEST_CASE("Page Setup effective text area in points", "[page_setup]") {
    SECTION("Letter landscape with 0.5in margins") {
        PageSettings ps;
        ps.applyPageSize(PageSize::Letter);
        ps.setOrientation(PageOrientation::Landscape);
        ps.setUniformMargins(36.0f);  // 0.5"
        REQUIRE(ps.textWidthPoints() == 720.0f);   // 792 - 36 - 36
        REQUIRE(ps.textHeightPoints() == 540.0f);  // 612 - 36 - 36
    }
    SECTION("per-side margins are independent") {
        PageSettings ps;
        ps.marginTop = 18.0f;
        ps.marginBottom = 54.0f;
        ps.marginLeft = 0.0f;
        ps.marginRight = 72.0f;
        REQUIRE(ps.textWidthPoints() == 540.0f);  // 612 - 0 - 72
        REQUIRE(ps.textHeightPoints() == 720.0f);  // 792 - 18 - 54
    }
}

TEST_CASE("Page Setup lines per column honors top+bottom margins",
          "[page_setup]") {
    SECTION("asymmetric margins use both, not a doubled single") {
        // Zero margins: the full 612pt page height at scale 1.
        REQUIRE(columns_math::linesPerColumnForPage(612.0f, 0.0f, 0.0f,
                                                     1.0f, 20) == 30);
        // 72pt top + 72pt bottom on the same page: 468 / 20 = 23.
        REQUIRE(columns_math::linesPerColumnForPage(612.0f, 72.0f, 72.0f,
                                                     1.0f, 20) == 23);
        // Top-only 144pt: same text height as 72+72, same answer.
        REQUIRE(columns_math::linesPerColumnForPage(612.0f, 144.0f, 0.0f,
                                                     1.0f, 20) == 23);
    }
    SECTION("uniform form delegates to the top+bottom form") {
        REQUIRE(columns_math::linesPerColumnForPage(792.0f, 72.0f, 0.5f,
                                                     13) ==
                columns_math::linesPerColumnForPage(792.0f, 72.0f, 72.0f,
                                                     0.5f, 13));
        REQUIRE(columns_math::linesPerColumnForPage(792.0f, 72.0f, 0.5f,
                                                     13) == 24);
    }
    SECTION("Letter landscape 0.5in margins") {
        // Text height 540pt at 0.5 scale = 270px / 13px lines = 20.
        REQUIRE(columns_math::linesPerColumnForPage(612.0f, 36.0f, 36.0f,
                                                     0.5f, 13) == 20);
    }
}

TEST_CASE("Page Setup print-preview estimate honors margins",
          "[page_setup]") {
    SECTION("landscape 0.5in margins fit fewer lines than portrait 1in") {
        // Portrait 1in: (792 - 144) / 20 = 32 lines per page.
        REQUIRE(scroll_math::previewLinesPerPage(648.0f, 20.0f) == 32);
        // Landscape 0.5in: (612 - 72) / 20 = 27 lines per page.
        REQUIRE(scroll_math::previewLinesPerPage(540.0f, 20.0f) == 27);
        REQUIRE(scroll_math::previewPageCount(100, 27) == 4);
        REQUIRE(scroll_math::previewPageCount(100, 32) == 4);
        REQUIRE(scroll_math::previewPageCount(200, 27) == 8);
        REQUIRE(scroll_math::previewPageCount(200, 32) == 7);
    }
}

TEST_CASE("Page Setup survives a save/load roundtrip", "[page_setup]") {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_pagesetup";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "setup.wpdoc").string();

    TextBuffer original;
    original.setText("page setup");
    DocumentSettings settings;
    settings.pageSettings.applyPageSize(PageSize::Letter);
    settings.pageSettings.setOrientation(PageOrientation::Landscape);
    settings.pageSettings.marginTop = 36.0f;
    settings.pageSettings.marginBottom = 36.0f;
    settings.pageSettings.marginLeft = 36.0f;
    settings.pageSettings.marginRight = 36.0f;
    REQUIRE(saveDocumentEx(original, settings, path).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    const PageSettings& page = loadedSettings.pageSettings;
    REQUIRE(page.size == PageSize::Letter);
    REQUIRE(page.orientation == PageOrientation::Landscape);
    REQUIRE(page.pageWidth == Approx(792.0f));
    REQUIRE(page.pageHeight == Approx(612.0f));
    REQUIRE(page.marginTop == Approx(36.0f));
    REQUIRE(page.marginBottom == Approx(36.0f));
    REQUIRE(page.marginLeft == Approx(36.0f));
    REQUIRE(page.marginRight == Approx(36.0f));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_CASE("Page Setup legacy files fall back to pageMargin",
          "[page_setup]") {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_pagesetup_legacy";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "legacy.wpdoc").string();
    writeLegacyPageSetupFile(path);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, path).success);
    const PageSettings& page = loadedSettings.pageSettings;
    REQUIRE(loaded.getText() == "legacy");
    REQUIRE(page.pageWidth == Approx(792.0f));
    REQUIRE(page.pageHeight == Approx(612.0f));
    REQUIRE(page.pageMargin == Approx(50.0f));
    REQUIRE(page.marginTop == Approx(50.0f));
    REQUIRE(page.marginBottom == Approx(50.0f));
    REQUIRE(page.marginLeft == Approx(50.0f));
    REQUIRE(page.marginRight == Approx(50.0f));
    // Size/orientation were never stored: defaults, not garbage.
    REQUIRE(page.size == PageSize::Letter);
    REQUIRE(page.orientation == PageOrientation::Portrait);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
