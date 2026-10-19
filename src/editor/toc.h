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
// Heading 2 -> 4, Heading 3 -> 6). The dot leader pads the entry to
// kEntryColumn so the page numbers line up; a heading longer than
// the column still gets at least three dots. Page numbers are the
// Batch 6 estimate: heading row / linesPerPage + 1 (1-based).

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

// Estimated page for a buffer row: row / linesPerPage + 1.
// linesPerPage <= 0 means one line per page.
inline int pageForLine(std::size_t line, int linesPerPage) {
    if (linesPerPage <= 0) linesPerPage = 1;
    return static_cast<int>(line / static_cast<std::size_t>(linesPerPage)) +
           1;
}

// One generated entry line: indent + text + dot leader + " " + page.
inline std::string entryLine(int level, const std::string& text, int page) {
    std::string content(static_cast<std::size_t>(level * 2), ' ');
    content += text;
    std::size_t dots = 3;
    if (content.size() < static_cast<std::size_t>(kEntryColumn)) {
        dots = static_cast<std::size_t>(kEntryColumn) - content.size();
    }
    content.append(dots, '.');
    content += ' ';
    content += std::to_string(page);
    return content;
}

inline std::string entryLine(const Entry& entry) {
    return entryLine(entry.level, entry.text, entry.page);
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
inline std::string blockText(const std::vector<Entry>& entries) {
    std::string out = std::string(kTitle) + '\n';
    for (const auto& entry : entries) {
        out += entryLine(entry);
        out += '\n';
    }
    return out;
}

}  // namespace toc
