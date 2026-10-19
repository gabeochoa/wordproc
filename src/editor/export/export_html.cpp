#include "export_html.h"

#include <cctype>
#include <fstream>
#include <sstream>

// HTML export walks the buffer's styled segments (Batch 13):
// each segment's effective style (span override, else the
// settings global style) maps to tags, and the body style
// carries the global font / size / color so a segment equal
// to the global style emits bare text. Bold / italic /
// underline / strikethrough / super / subscript are tags
// (<b> <i> <u> <s> <sup> <sub>) taken from the segment's
// absolute style; font / size / color / highlight ride in
// a <span style="..."> only when they differ from the
// global style (highlight: whenever the segment has one,
// since the body carries no highlight).

static std::string cssColor(const TextColor& color) {
    if (color.a == 255) {
        return "rgb(" + std::to_string(color.r) + ", " +
               std::to_string(color.g) + ", " + std::to_string(color.b) +
               ")";
    }
    std::ostringstream out;
    out << "rgba(" << static_cast<int>(color.r) << ", "
        << static_cast<int>(color.g) << ", " << static_cast<int>(color.b)
        << ", " << (static_cast<double>(color.a) / 255.0) << ")";
    return out.str();
}

static std::string cssFontFamily(const std::string& font) {
    // Quote only when the name needs it (spaces / punctuation);
    // plain names (Arial, Gaegu-Bold) stay bare.
    bool simple = !font.empty();
    for (char ch : font) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' ||
              ch == '_')) {
            simple = false;
            break;
        }
    }
    if (simple) return font;
    std::string out = "'";
    for (char ch : font) {
        if (ch == '\'' || ch == '\\') out.push_back('\\');
        out.push_back(ch);
    }
    out.push_back('\'');
    return out;
}

static void appendEscapedHtml(std::ostringstream& out,
                              const std::string& text) {
    for (char ch : text) {
        switch (ch) {
            case '&': out << "&amp;"; break;
            case '<': out << "&lt;"; break;
            case '>': out << "&gt;"; break;
            case '"': out << "&quot;"; break;
            default: out.put(ch); break;
        }
    }
}

static void appendSegmentHtml(std::ostringstream& out,
                              const std::string& text,
                              const TextStyle& style,
                              const TextStyle& base) {
    std::string decls;
    auto addDecl = [&decls](const std::string& decl) {
        if (!decls.empty()) decls += "; ";
        decls += decl;
    };
    if (style.font != base.font) {
        addDecl("font-family: " + cssFontFamily(style.font));
    }
    if (style.fontSize != base.fontSize) {
        addDecl("font-size: " + std::to_string(style.fontSize) + "px");
    }
    if (style.textColor != base.textColor) {
        addDecl("color: " + cssColor(style.textColor));
    }
    if (!style.highlightColor.isNone()) {
        addDecl("background-color: " + cssColor(style.highlightColor));
    }
    std::string open;
    std::string close;
    if (!decls.empty()) {
        open += "<span style=\"" + decls + "\">";
        close = "</span>" + close;
    }
    if (style.bold) {
        open += "<b>";
        close = "</b>" + close;
    }
    if (style.italic) {
        open += "<i>";
        close = "</i>" + close;
    }
    if (style.underline) {
        open += "<u>";
        close = "</u>" + close;
    }
    if (style.strikethrough) {
        open += "<s>";
        close = "</s>" + close;
    }
    if (style.superscript) {
        open += "<sup>";
        close = "</sup>" + close;
    } else if (style.subscript) {
        open += "<sub>";
        close = "</sub>" + close;
    }
    out << open;
    appendEscapedHtml(out, text);
    out << close;
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

    const TextStyle& base = settings.textStyle;
    std::ostringstream body;
    for (std::size_t i = 0; i < buffer.lineCount(); ++i) {
        LineSpan span = buffer.lineSpan(i);
        std::string line = buffer.lineString(i);
        auto segments = buffer.styledSegments(span.offset,
                                              span.offset + span.length, base);
        if (segments.empty()) {
            // Empty line: nothing to emit (the '\n' below is its line).
        } else {
            for (const auto& seg : segments) {
                appendSegmentHtml(
                    body,
                    line.substr(seg.startOffset - span.offset,
                                seg.endOffset - seg.startOffset),
                    seg.style, base);
            }
        }
        body << '\n';
    }

    out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\"/>\n";
    out << "<title>Wordproc Export</title>\n";
    out << "<style>body{font-family:" << cssFontFamily(base.font)
        << ";font-size:" << base.fontSize
        << "px;color:" << cssColor(base.textColor)
        << ";white-space:pre-wrap;}</style>\n";
    out << "</head>\n<body>\n";
    out << body.str();
    out << "\n</body>\n</html>\n";

    result.success = true;
    return result;
}
