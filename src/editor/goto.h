// Go To math (Batch 9, Edit > Go To... / Ctrl+G) — pure
// functions with no UI/ECS dependency, so the unit tests can
// pin the edge cases directly (the same reason scroll_math
// and columns_math are light headers). cmd::performGoTo in
// document_commands.h applies these to the live buffer.
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

namespace goto_math {

// Parse a positive 1-based number from the Go To field:
// digits only (leading/trailing whitespace tolerated),
// clamped to int range; returns -1 when not a number
// ("", "0", "abc", "12a", "-3", "1.5", overflow all fail).
inline int parseGoToNumber(const std::string& input) {
    std::size_t begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return -1;
    std::size_t end = input.find_last_not_of(" \t\r\n");
    long value = 0;
    for (std::size_t i = begin; i <= end; ++i) {
        if (input[i] < '0' || input[i] > '9') return -1;
        value = value * 10 + (input[i] - '0');
        if (value > 1000000000L) return -1;
    }
    if (value < 1) return -1;
    return static_cast<int>(value);
}

// First 0-based line of a 1-based page: page clamped to
// [1, totalPages], then (page - 1) * linesPerPage, clamped
// to the last line. An empty document has one (empty) line.
inline std::size_t firstLineOfPage(std::size_t lineCount,
                                   std::size_t linesPerPage, int page) {
    if (lineCount == 0) return 0;
    if (linesPerPage == 0) linesPerPage = 1;
    std::size_t totalPages =
        (lineCount + linesPerPage - 1) / linesPerPage;
    std::size_t clampedPage =
        static_cast<std::size_t>(std::clamp(page, 1,
                                            static_cast<int>(totalPages)));
    std::size_t first = (clampedPage - 1) * linesPerPage;
    return std::min(first, lineCount - 1);
}

// 1-based line number -> 0-based row, clamped to the last line.
inline std::size_t clampedLine(std::size_t lineCount, int line) {
    if (lineCount == 0) return 0;
    if (line < 1) return 0;
    return std::min(static_cast<std::size_t>(line - 1), lineCount - 1);
}

}  // namespace goto_math
