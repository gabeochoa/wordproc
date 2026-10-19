#include <filesystem>
#include <fstream>
#include <sstream>

#include "../src/editor/document_io.h"
#include "../src/editor/export/export_html.h"
#include "../src/editor/export/export_pdf.h"
#include "../src/editor/export/export_rtf.h"
#include "catch2/catch.hpp"

namespace {

std::filesystem::path testDir() {
    auto dir = std::filesystem::temp_directory_path() / "wordproc_test_export";
    std::filesystem::create_directories(dir);
    return dir;
}

struct TestDirGuard {
    std::filesystem::path dir;
    TestDirGuard() : dir(testDir()) {}
    ~TestDirGuard() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Doc: "plain Bold Italic Red Under Strike Super Sub Big Tail"
// with one span per styled word; everything else is the
// (plain) global style.
TextBuffer styledDoc() {
    TextBuffer buffer;
    const std::string text =
        "plain Bold Italic Red Under Strike Super Sub Big Tail";
    buffer.setText(text);
    auto at = [&text](const std::string& word) {
        return text.find(word);
    };
    auto apply = [&](const std::string& word, const TextStyle& style) {
        std::size_t start = at(word);
        buffer.applyCharStyleRange(start, start + word.size(), style);
    };
    TextStyle bold;
    bold.bold = true;
    apply("Bold", bold);
    TextStyle italic;
    italic.italic = true;
    apply("Italic", italic);
    TextStyle red;
    red.textColor = TextColors::Red;
    apply("Red", red);
    TextStyle under;
    under.underline = true;
    apply("Under", under);
    TextStyle strike;
    strike.strikethrough = true;
    apply("Strike", strike);
    TextStyle superStyle;
    superStyle.superscript = true;
    apply("Super", superStyle);
    TextStyle sub;
    sub.subscript = true;
    apply("Sub", sub);
    TextStyle big;
    big.font = "Arial";
    big.fontSize = 20;
    apply("Big", big);
    return buffer;
}

// The last "/F<n> " font selection at or before pos in a PDF.
std::string fontBefore(const std::string& pdf, std::size_t pos) {
    std::string found;
    for (const char* f : {"/F1 ", "/F2 ", "/F3 ", "/F4 "}) {
        std::size_t at = pdf.rfind(f, pos);
        if (at != std::string::npos &&
            (found.empty() || at > pdf.rfind(found, pos))) {
            found = f;
        }
    }
    return found;
}

}  // namespace

TEST_CASE("styledSegments iterator", "[export]") {
    TextBuffer buffer;
    buffer.setText("alpha beta gamma");
    TextStyle bold;
    bold.bold = true;
    TextStyle italic;
    italic.italic = true;
    buffer.applyCharStyleRange(0, 5, bold);
    buffer.applyCharStyleRange(11, 16, italic);

    SECTION("splits at span boundaries, base between spans") {
        auto segs = buffer.styledSegments(0, 16);
        REQUIRE(segs.size() == 3);
        REQUIRE(segs[0].startOffset == 0);
        REQUIRE(segs[0].endOffset == 5);
        REQUIRE(segs[0].style.bold);
        REQUIRE(segs[1].startOffset == 5);
        REQUIRE(segs[1].endOffset == 11);
        REQUIRE_FALSE(segs[1].style.bold);
        REQUIRE_FALSE(segs[1].style.italic);
        REQUIRE(segs[2].startOffset == 11);
        REQUIRE(segs[2].endOffset == 16);
        REQUIRE(segs[2].style.italic);
    }

    SECTION("clips to the requested range") {
        auto segs = buffer.styledSegments(3, 8);
        REQUIRE(segs.size() == 2);
        REQUIRE(segs[0].startOffset == 3);
        REQUIRE(segs[0].endOffset == 5);
        REQUIRE(segs[0].style.bold);
        REQUIRE(segs[1].startOffset == 5);
        REQUIRE(segs[1].endOffset == 8);
    }

    SECTION("empty range is empty; out-of-range clips") {
        REQUIRE(buffer.styledSegments(5, 5).empty());
        REQUIRE(buffer.styledSegments(14, 99).size() == 1);
        REQUIRE(buffer.styledSegments(14, 99)[0].endOffset == 16);
    }

    SECTION("caller-supplied base fills unspanned runs") {
        TextStyle base;
        base.fontSize = 99;
        auto segs = buffer.styledSegments(0, 16, base);
        REQUIRE(segs.size() == 3);
        REQUIRE(segs[1].style.fontSize == 99);
        // Spans are full overrides, not layered on the base.
        REQUIRE(segs[0].style.fontSize == 16);
    }

    SECTION("a span equal to the base merges into one run") {
        TextBuffer plain;
        plain.setText("alpha beta gamma");
        plain.applyCharStyleRange(0, 5, plain.textStyle());
        auto segs = plain.styledSegments(0, 16);
        REQUIRE(segs.size() == 1);
        REQUIRE(segs[0].startOffset == 0);
        REQUIRE(segs[0].endOffset == 16);
    }
}

TEST_CASE("Export honors CharStyleSpans", "[export]") {
    TestDirGuard guard;
    TextBuffer buffer = styledDoc();
    DocumentSettings settings;
    settings.textStyle = buffer.textStyle();

    SECTION("HTML wraps each styled word, plain neighbours bare") {
        std::string path = (guard.dir / "out.html").string();
        REQUIRE(exportDocumentHtml(buffer, settings, path).success);
        std::string html = readFile(path);
        REQUIRE(html.find("plain <b>Bold</b>") != std::string::npos);
        REQUIRE(html.find("<i>Italic</i>") != std::string::npos);
        REQUIRE(html.find("<span style=\"color: rgb(200, 0, 0)\">Red</span>") !=
                std::string::npos);
        REQUIRE(html.find("<u>Under</u>") != std::string::npos);
        REQUIRE(html.find("<s>Strike</s>") != std::string::npos);
        REQUIRE(html.find("<sup>Super</sup>") != std::string::npos);
        REQUIRE(html.find("<sub>Sub</sub>") != std::string::npos);
        REQUIRE(html.find("<span style=\"font-family: Arial; "
                          "font-size: 20px\">Big</span>") !=
                std::string::npos);
        REQUIRE(html.find("Big</span> Tail") != std::string::npos);
    }

    SECTION("RTF emits control words around the right text") {
        std::string path = (guard.dir / "out.rtf").string();
        REQUIRE(exportDocumentRtf(buffer, settings, path).success);
        std::string rtf = readFile(path);
        REQUIRE(rtf.find("\\b Bold") != std::string::npos);
        REQUIRE(rtf.find("\\i Italic") != std::string::npos);
        REQUIRE(rtf.find("\\cf2 Red") != std::string::npos);
        REQUIRE(rtf.find("\\ul Under") != std::string::npos);
        REQUIRE(rtf.find("\\strike Strike") != std::string::npos);
        REQUIRE(rtf.find("\\super Super") != std::string::npos);
        REQUIRE(rtf.find("\\sub Sub") != std::string::npos);
        REQUIRE(rtf.find("\\f1 \\fs40 Big") != std::string::npos);
        REQUIRE(rtf.find("{\\f0 Gaegu-Bold;}{\\f1 Arial;}") !=
                std::string::npos);
        REQUIRE(rtf.find("\\red200\\green0\\blue0;") != std::string::npos);
        // The plain space after Bold turns bold off again.
        REQUIRE(rtf.find("\\b0 ") != std::string::npos);
    }

    SECTION("PDF switches font / size / color per segment") {
        std::string path = (guard.dir / "out.pdf").string();
        REQUIRE(exportDocumentPdf(buffer, settings, path).success);
        std::string pdf = readFile(path);
        REQUIRE(pdf.find("/BaseFont /Helvetica-Bold") != std::string::npos);
        REQUIRE(pdf.find("/BaseFont /Helvetica-Oblique") !=
                std::string::npos);
        std::size_t boldAt = pdf.find("(Bold)");
        REQUIRE(boldAt != std::string::npos);
        REQUIRE(fontBefore(pdf, boldAt) == "/F2 ");
        std::size_t italicAt = pdf.find("(Italic)");
        REQUIRE(italicAt != std::string::npos);
        REQUIRE(fontBefore(pdf, italicAt) == "/F3 ");
        std::size_t plainAt = pdf.find("(plain ");
        REQUIRE(plainAt != std::string::npos);
        REQUIRE(fontBefore(pdf, plainAt) == "/F1 ");
        std::size_t redAt = pdf.find("(Red)");
        REQUIRE(redAt != std::string::npos);
        std::size_t redColor = pdf.find("0.784 0.000 0.000 rg");
        REQUIRE(redColor != std::string::npos);
        REQUIRE(redColor < redAt);
        REQUIRE(redColor > italicAt);
        std::size_t superAt = pdf.find("(Super)");
        REQUIRE(superAt != std::string::npos);
        REQUIRE(fontBefore(pdf, superAt) == "/F1 ");
        REQUIRE(pdf.find("/F1 12 Tf") != std::string::npos);
        REQUIRE(pdf.find("0 5 Td") != std::string::npos);
        std::size_t bigAt = pdf.find("(Big)");
        REQUIRE(bigAt != std::string::npos);
        REQUIRE(pdf.find("/F1 20 Tf") != std::string::npos);
        REQUIRE(pdf.rfind("/F1 20 Tf", bigAt) != std::string::npos);
    }
}

TEST_CASE("Save then export keeps per-range styles", "[export][document_io]") {
    TestDirGuard guard;
    TextBuffer original = styledDoc();
    DocumentSettings settings;
    settings.textStyle = original.textStyle();
    std::string docPath = (guard.dir / "doc.wpdoc").string();
    REQUIRE(saveDocumentEx(original, settings, docPath).success);

    TextBuffer loaded;
    DocumentSettings loadedSettings;
    REQUIRE(loadDocumentEx(loaded, loadedSettings, docPath).success);
    REQUIRE(loaded.getText() == original.getText());
    REQUIRE(loaded.charStyleSpans().size() ==
            original.charStyleSpans().size());

    std::string htmlPath = (guard.dir / "roundtrip.html").string();
    REQUIRE(exportDocumentHtml(loaded, loadedSettings, htmlPath).success);
    std::string html = readFile(htmlPath);
    REQUIRE(html.find("plain <b>Bold</b>") != std::string::npos);
    REQUIRE(html.find("<span style=\"color: rgb(200, 0, 0)\">Red</span>") !=
            std::string::npos);

    std::string rtfPath = (guard.dir / "roundtrip.rtf").string();
    REQUIRE(exportDocumentRtf(loaded, loadedSettings, rtfPath).success);
    REQUIRE(readFile(rtfPath).find("\\b Bold") != std::string::npos);

    std::string pdfPath = (guard.dir / "roundtrip.pdf").string();
    REQUIRE(exportDocumentPdf(loaded, loadedSettings, pdfPath).success);
    std::string pdf = readFile(pdfPath);
    std::size_t boldAt = pdf.find("(Bold)");
    REQUIRE(boldAt != std::string::npos);
    REQUIRE(fontBefore(pdf, boldAt) == "/F2 ");
}
