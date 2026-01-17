#include "export_rtf.h"

#include <fstream>

static std::string escapeRtf(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '{': out += "\\{"; break;
            case '}': out += "\\}"; break;
            case '\n': out += "\\par\n"; break;
            default: out += ch; break;
        }
    }
    return out;
}

DocumentResult exportDocumentRtf(const TextBuffer& buffer,
                                 const DocumentSettings& settings,
                                 const std::string& path) {
    DocumentResult result;
    std::ofstream out(path);
    if (!out.is_open()) {
        result.success = false;
        result.error = "Failed to open file for writing";
        return result;
    }

    std::string text = buffer.getText();
    out << "{\\rtf1\\ansi\\deff0\n";
    out << "{\\fonttbl{\\f0 " << settings.textStyle.font << ";}}\n";
    out << "\\fs" << (settings.textStyle.fontSize * 2) << " ";
    out << escapeRtf(text);
    out << "\n}\n";

    result.success = true;
    return result;
}

