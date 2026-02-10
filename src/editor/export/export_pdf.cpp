#include "export_pdf.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

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

    std::ostringstream content;
    content << "BT\n/F1 " << fontSize << " Tf\n";
    content << "72 720 Td\n";
    std::size_t numLines = buffer.lineCount();
    for (std::size_t i = 0; i < numLines; ++i) {
        content << "(";
        auto view = buffer.lineView(i);
        if (view) {
            // Zero-copy: escape directly from buffer pointer
            for (std::size_t j = 0; j < view.length; ++j) {
                char ch = view.data[j];
                if (ch == '(' || ch == ')' || ch == '\\') {
                    content.put('\\');
                }
                content.put(ch);
            }
        } else {
            content << escapePdfText(buffer.lineString(i));
        }
        content << ") Tj\n";
        if (i + 1 < numLines) {
            content << "0 -" << lineHeight << " Td\n";
        }
    }
    content << "ET\n";
    std::string contentStr = content.str();

    std::vector<std::string> objects;
    objects.push_back("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
    objects.push_back("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
    objects.push_back("3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
                      "/Contents 5 0 R /Resources << /Font << /F1 4 0 R >> >> >>\nendobj\n");
    objects.push_back("4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n");

    std::ostringstream contentsObj;
    contentsObj << "5 0 obj\n<< /Length " << contentStr.size() << " >>\nstream\n"
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

