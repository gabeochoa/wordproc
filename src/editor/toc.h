#pragma once

// Table of Contents (Insert > Table of Contents): pure helpers over
// plain values, so unit tests pin the generated text without a
// TextBuffer. The TextBuffer owns the marked region (see
// TextBuffer::insertTableOfContents / updateTableOfContents);
// this header only formats it.
//
// Format (one line per Title / Heading 1-3 paragraph):
//   <indent><heading text><dot leader> <page>
// Indent is two spaces per outline level (Title 0, Heading 1 -> 2,
// Heading 2 -> 4, Heading 3 -> 6). The dot leader fills from the
// end of the heading text to the tab stop at the text area's
// right edge, so the page number is right-aligned: with a
// Layout (measured at generation time) the dots are counted in
// pixels — measure(prefix + dots + " " + page) lands within one
// dot width of Layout::targetWidthPx, never over it (except a
// heading so long that even the minimum three dots overshoot).
// Without a Layout the legacy kEntryColumn character column is
// used, so callers that cannot measure (unit tests for the other
// TOC mechanics, save/load) keep the old deterministic output.
// Regeneration recomputes the layout from the current text area,
// so a resize / zoom / margin change re-flows the leaders on the
// next Update Table of Contents. Page numbers are the Batch 6
// estimate: heading row / linesPerPage + 1 (1-based).

#include <functional>
#include <string>
#include <vector>

namespace toc {

inline constexpr const char* kTitle = "Table of Contents";
inline constexpr int kEntryColumn = 44;
inline constexpr int kDefaultLinesPerPage = 50;

struct Entry {
    std::string text;
    int level = 0;                 // Outline level: Title 0, H1 1, H2 2, H3 3
    std::size_t lineNumber = 0;     // Buffer row of the heading
    int page = 1;                  // Estimated page (1-based)
};

// Measured generation layout: the tab-stop width in pixels and
// the measure to count dots against. measure maps a string to
// its pixel width at the body (Normal) font size — production
// passes theme::MeasureUIText bound to that size; tests pass a
// stand-in. targetWidthPx <= 0 or an empty measure means
// "no layout": the legacy character column applies.
struct Layout {
    int targetWidthPx = 0;
    std::function<int(const std::string&)> measure;
};

// Estimated page for a buffer row: row / linesPerPage + 1.
// linesPerPage <= 0 means one line per page.
inline int pageForLine(std::size_t line, int linesPerPage) {
    if (linesPerPage <= 0) linesPerPage = 1;
    return static_cast<int>(line / static_cast<std::size_t>(linesPerPage)) +
           1;
}

// One generated entry line: indent + text + dot leader + " " + page.
inline std::string entryLine(int level, const std::string& text, int page,
                             const Layout& layout = Layout{}) {
    std::string content(static_cast<std::size_t>(level * 2), ' ');
    content += text;
    const std::string pageStr = std::to_string(page);
    std::size_t dots = 3;
    if (layout.measure && layout.targetWidthPx > 0) {
        const int dotW = layout.measure(".");
        if (dotW > 0) {
            const int used = layout.measure(content) +
                             layout.measure(" ") + layout.measure(pageStr);
            const int room = layout.targetWidthPx - used;
            if (room >= 0) {
                dots = static_cast<std::size_t>(room / dotW);
            } else {
                dots = 0;
            }
            if (dots < 3) dots = 3;
        }
    } else if (content.size() < static_cast<std::size_t>(kEntryColumn)) {
        dots = static_cast<std::size_t>(kEntryColumn) - content.size();
    }
    content.append(dots, '.');
    content += ' ';
    content += pageStr;
    return content;
}

inline std::string entryLine(const Entry& entry,
                             const Layout& layout = Layout{}) {
    return entryLine(entry.level, entry.text, entry.page, layout);
}

// Does this line look like a generated TOC entry? At least three
// dots, then a space, then trailing digits; something (the heading
// text) before the dots. Hand edits that keep the dots + page keep
// matching; edits that drop them stop matching, so the rescan in
// TextBuffer::resolveTocRange stops at that line.
inline bool isEntryLine(const std::string& line) {
    if (line.empty()) return false;
    std::size_t end = line.size();
    while (end > 0 &&
           line[end - 1] >= '0' && line[end - 1] <= '9') {
        --end;
    }
    if (end == line.size() || end == 0) return false;  // No trailing digits
    if (line[end - 1] != ' ') return false;
    std::size_t dotsEnd = end - 1;
    std::size_t dots = 0;
    while (dotsEnd > 0 && line[dotsEnd - 1] == '.') {
        --dotsEnd;
        ++dots;
    }
    if (dots < 3) return false;
    // Heading text before the dots (ignoring the indent).
    std::size_t textStart = line.find_first_not_of(' ');
    return textStart != std::string::npos && textStart < dotsEnd;
}

// The whole generated block: title line + one entry line each.
// Every line ends with '\n', including the last entry, so the block
// is a whole number of lines and its end offset sits at the start
// of the following line (or EOF).
inline std::string blockText(const std::vector<Entry>& entries,
                             const Layout& layout = Layout{}) {
    std::string out = std::string(kTitle) + '\n';
    for (const auto& entry : entries) {
        out += entryLine(entry, layout);
        out += '\n';
    }
    return out;
}

}  // namespace toc
