#include "export_rtf.h"

#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

// RTF export walks the buffer's styled segments (Batch 13):
// each segment's effective style (span override, else the
// settings global style) emits control words, statefully —
// only the controls that change from the current state are
// emitted, so a plain neighbour after a bold word gets
// \b0 and a bold word gets \b. Fonts extend the font table
// (\fN, base font stays \f0) and text + highlight colors
// extend the color table (\cfN / \highlightN, entry 0 is
// the RTF auto color).

static void appendEscapedRtf(std::ostream& out, const std::string& text) {
    for (char ch : text) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '{': out << "\\{"; break;
            case '}': out << "\\}"; break;
            case '\n': out << "\\par\n"; break;
            default: out.put(ch); break;
        }
    }
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

    const TextStyle& base = settings.textStyle;

    // Collect the font + color tables first: base entries first
    // (stable indices: base font \f0, base color \cf1), then
    // segment values in first-appearance order.
    std::vector<std::string> fonts{base.font};
    std::map<std::string, std::size_t> fontIndex{{base.font, 0}};
    std::vector<TextColor> colors{base.textColor};
    std::map<std::tuple<int, int, int, int>, std::size_t> colorIndex{
        {{base.textColor.r, base.textColor.g, base.textColor.b,
          base.textColor.a},
         1}};
    auto internFont = [&](const std::string& font) {
        auto it = fontIndex.find(font);
        if (it != fontIndex.end()) return it->second;
        std::size_t idx = fonts.size();
        fonts.push_back(font);
        fontIndex[font] = idx;
        return idx;
    };
    auto internColor = [&](const TextColor& color) {
        auto key = std::make_tuple(static_cast<int>(color.r),
                                   static_cast<int>(color.g),
                                   static_cast<int>(color.b),
                                   static_cast<int>(color.a));
        auto it = colorIndex.find(key);
        if (it != colorIndex.end()) return it->second;
        std::size_t idx = colors.size() + 1;  // +1: entry 0 is auto.
        colors.push_back(color);
        colorIndex[key] = idx;
        return idx;
    };
    for (std::size_t i = 0; i < buffer.lineCount(); ++i) {
        LineSpan span = buffer.lineSpan(i);
        for (const auto& seg :
             buffer.styledSegments(span.offset, span.offset + span.length,
                                   base)) {
            internFont(seg.style.font);
            internColor(seg.style.textColor);
            if (!seg.style.highlightColor.isNone()) {
                internColor(seg.style.highlightColor);
            }
        }
    }

    out << "{\\rtf1\\ansi\\deff0\n";
    out << "{\\fonttbl";
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        out << "{\\f" << i << " " << fonts[i] << ";}";
    }
    out << "}\n";
    out << "{\\colortbl ;";
    for (const auto& color : colors) {
        out << "\\red" << static_cast<int>(color.r) << "\\green"
            << static_cast<int>(color.g) << "\\blue"
            << static_cast<int>(color.b) << ";";
    }
    out << "}\n";

    // Emit the base state, then per-segment diffs against it.
    auto emitState = [&](const TextStyle& style, const TextStyle* prev) {
        if (prev == nullptr || style.font != prev->font) {
            out << "\\f" << fontIndex[style.font] << " ";
        }
        if (prev == nullptr || style.fontSize != prev->fontSize) {
            out << "\\fs" << (style.fontSize * 2) << " ";
        }
        if (prev == nullptr || style.textColor != prev->textColor) {
            out << "\\cf" << internColor(style.textColor) << " ";
        }
        if (prev == nullptr || style.highlightColor != prev->highlightColor) {
            if (style.highlightColor.isNone()) {
                out << "\\highlight0 ";
            } else {
                out << "\\highlight" << internColor(style.highlightColor)
                    << " ";
            }
        }
        if (prev == nullptr || style.bold != prev->bold) {
            out << (style.bold ? "\\b " : "\\b0 ");
        }
        if (prev == nullptr || style.italic != prev->italic) {
            out << (style.italic ? "\\i " : "\\i0 ");
        }
        if (prev == nullptr || style.underline != prev->underline) {
            out << (style.underline ? "\\ul " : "\\ulnone ");
        }
        if (prev == nullptr || style.strikethrough != prev->strikethrough) {
            out << (style.strikethrough ? "\\strike " : "\\strike0 ");
        }
        bool prevSuper = prev != nullptr && prev->superscript;
        bool prevSub = prev != nullptr && prev->subscript;
        if (prev == nullptr || style.superscript != prevSuper ||
            style.subscript != prevSub) {
            if (style.superscript) {
                out << "\\super ";
            } else if (style.subscript) {
                out << "\\sub ";
            } else {
                out << "\\nosupersub ";
            }
        }
    };
    emitState(base, nullptr);
    TextStyle current = base;

    for (std::size_t i = 0; i < buffer.lineCount(); ++i) {
        if (i > 0) out << "\\par\n";
        LineSpan span = buffer.lineSpan(i);
        std::string line = buffer.lineString(i);
        for (const auto& seg :
             buffer.styledSegments(span.offset, span.offset + span.length,
                                   base)) {
            emitState(seg.style, &current);
            current = seg.style;
            appendEscapedRtf(
                out, line.substr(seg.startOffset - span.offset,
                                 seg.endOffset - seg.startOffset));
        }
    }
    out << "\n}\n";

    result.success = true;
    return result;
}
