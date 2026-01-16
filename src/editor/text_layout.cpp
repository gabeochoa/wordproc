#include "text_layout.h"

#include <algorithm>

std::vector<WrappedLine> layoutWrappedLines(const TextBuffer &buffer,
                                            std::size_t max_columns) {
  std::vector<WrappedLine> result;
  if (max_columns == 0) {
    return result;
  }

  const auto &lines = buffer.lines();
  for (std::size_t row = 0; row < lines.size(); ++row) {
    const std::string &line = lines[row];
    if (line.empty()) {
      result.push_back(WrappedLine{row, 0, 0, ""});
      continue;
    }

    std::size_t start = 0;
    while (start < line.size()) {
      std::size_t len = std::min(max_columns, line.size() - start);
      result.push_back(
          WrappedLine{row, start, len, line.substr(start, len)});
      start += len;
    }
  }

  return result;
}

// SoA layout implementation - avoids string copies
// Uses parallel arrays for better cache locality during iteration
LayoutResult layoutWrappedLinesSoA(const TextBuffer &buffer,
                                   std::size_t max_columns) {
  LayoutResult result;
  if (max_columns == 0) {
    return result;
  }

  const auto &lines = buffer.lines();
  
  // Pre-compute total wrapped lines for reservation
  std::size_t estimated_count = 0;
  for (const auto &line : lines) {
    estimated_count += line.empty() ? 1 : (line.size() + max_columns - 1) / max_columns;
  }
  result.reserve(estimated_count);

  for (std::size_t row = 0; row < lines.size(); ++row) {
    const std::string &line = lines[row];
    if (line.empty()) {
      result.push_back(row, 0, 0);
      continue;
    }

    std::size_t start = 0;
    while (start < line.size()) {
      std::size_t len = std::min(max_columns, line.size() - start);
      result.push_back(row, start, len);
      start += len;
    }
  }

  return result;
}
