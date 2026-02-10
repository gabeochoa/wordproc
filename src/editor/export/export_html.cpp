#include "export_html.h"

#include <fstream>
#include <sstream>

static std::string escapeHtml(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += ch; break;
        }
    }
    return out;
}

DocumentResult exportDocumentHtml(const TextBuffer& buffer,
                                  const DocumentSettings& settings,
                                  const std::string& path) {
    DocumentResult result;
    std::ofstream out(path);
    if (!out.is_open()) {
        result.success = false;
        result.error = "Failed to open file for writing";
        return result;
    }

    std::ostringstream body;
    for (std::size_t i = 0; i < buffer.lineCount(); ++i) {
        auto view = buffer.lineView(i);
        if (view) {
            // Zero-copy: escape directly from buffer pointer
            for (std::size_t j = 0; j < view.length; ++j) {
                char ch = view.data[j];
                switch (ch) {
                    case '&': body << "&amp;"; break;
                    case '<': body << "&lt;"; break;
                    case '>': body << "&gt;"; break;
                    case '"': body << "&quot;"; break;
                    default: body.put(ch); break;
                }
            }
        } else {
            body << escapeHtml(buffer.lineString(i));
        }
        body << '\n';
    }

    out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\"/>\n";
    out << "<title>Wordproc Export</title>\n";
    out << "<style>body{font-family:sans-serif;font-size:"
        << settings.textStyle.fontSize << "px;white-space:pre-wrap;}</style>\n";
    out << "</head>\n<body>\n";
    out << body.str();
    out << "\n</body>\n</html>\n";

    result.success = true;
    return result;
}

