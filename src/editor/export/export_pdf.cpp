#include "export_pdf.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

// PDF export walks the buffer's styled segments (Batch 13):
// each segment selects its own Type1 font (Helvetica /
// Helvetica-Bold / Helvetica-Oblique / Helvetica-BoldOblique
// for the segment's bold+italic flags), size, and color
// before its (text) Tj, so a bold word and a plain neighbour
// draw in different fonts within one line's text run.
// Underline / strikethrough have no Tj-level operator, and
// the exporter carries no font metrics to place rules, so
// they are not drawn here (HTML/RTF carry them). Super /
// subscript shrink to 3/4 size and shift the baseline with
// Td, restored after the segment's Tj.

static std::string escapePdfText(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        if (ch == '(' || ch == ')' || ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

static std::string pdfColor(const TextColor& color) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3)
        << (static_cast<double>(color.r) / 255.0) << " "
        << (static_cast<double>(color.g) / 255.0) << " "
        << (static_cast<double>(color.b) / 255.0) << " rg";
    return out.str();
}

DocumentResult exportDocumentPdf(const TextBuffer& buffer,
                                 const DocumentSettings& settings,
                                 const std::string& path) {
    DocumentResult result;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        result.success = false;
        result.error = "Failed to open file for writing";
        return result;
    }

    int fontSize = settings.textStyle.fontSize;
    if (fontSize <= 0) fontSize = 12;
    int lineHeight = fontSize + 4;
    const TextStyle& base = settings.textStyle;

    std::ostringstream content;
    content << "BT\n";
    content << "72 720 Td\n";
    std::size_t numLines = buffer.lineCount();
    for (std::size_t i = 0; i < numLines; ++i) {
        LineSpan span = buffer.lineSpan(i);
        std::string line = buffer.lineString(i);
        auto segments = buffer.styledSegments(span.offset,
                                              span.offset + span.length, base);
        if (segments.empty()) {
            content << "() Tj\n";
        } else {
            for (const auto& seg : segments) {
                std::string segText = line.substr(
                    seg.startOffset - span.offset,
                    seg.endOffset - seg.startOffset);
                if (segText.empty()) continue;
                const char* fontRef = "/F1";
                if (seg.style.bold && seg.style.italic) {
                    fontRef = "/F4";
                } else if (seg.style.bold) {
                    fontRef = "/F2";
                } else if (seg.style.italic) {
                    fontRef = "/F3";
                }
                int segSize = seg.style.fontSize > 0 ? seg.style.fontSize
                                                     : fontSize;
                int rise = 0;
                if (seg.style.superscript || seg.style.subscript) {
                    segSize = std::max(8, static_cast<int>(segSize * 0.75f));
                    rise = seg.style.superscript ? fontSize / 3
                                                 : -fontSize / 4;
                }
                content << fontRef << " " << segSize << " Tf\n";
                content << pdfColor(seg.style.textColor) << "\n";
                if (rise != 0) content << "0 " << rise << " Td\n";
                content << "(" << escapePdfText(segText) << ") Tj\n";
                if (rise != 0) content << "0 " << -rise << " Td\n";
            }
        }
        if (i + 1 < numLines) {
            content << "0 -" << lineHeight << " Td\n";
        }
    }
    content << "ET\n";
    std::string contentStr = content.str();

    // Objects: 1 catalog, 2 pages, 3 page, 4-7 fonts, 8 contents.
    std::vector<std::string> objects;
    objects.push_back("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
    objects.push_back("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
    objects.push_back(
        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Contents 8 0 R /Resources << /Font << /F1 4 0 R /F2 5 0 R "
        "/F3 6 0 R /F4 7 0 R >> >> >>\nendobj\n");
    objects.push_back(
        "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica "
        ">>\nendobj\n");
    objects.push_back(
        "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont "
        "/Helvetica-Bold >>\nendobj\n");
    objects.push_back(
        "6 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont "
        "/Helvetica-Oblique >>\nendobj\n");
    objects.push_back(
        "7 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont "
        "/Helvetica-BoldOblique >>\nendobj\n");

    std::ostringstream contentsObj;
    contentsObj << "8 0 obj\n<< /Length " << contentStr.size() << " >>\nstream\n"
                << contentStr << "endstream\nendobj\n";
    objects.push_back(contentsObj.str());

    out << "%PDF-1.4\n";
    std::vector<std::streamoff> offsets;
    offsets.reserve(objects.size() + 1);
    offsets.push_back(0);

    for (const auto& obj : objects) {
        offsets.push_back(out.tellp());
        out << obj;
    }

    std::streamoff xrefOffset = out.tellp();
    out << "xref\n0 " << (objects.size() + 1) << "\n";
    out << "0000000000 65535 f \n";
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        out << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
    }
    out << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\n";
    out << "startxref\n" << xrefOffset << "\n%%EOF\n";

    result.success = true;
    return result;
}
