#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "text_buffer.h"

// SoA layout: WrappedLine stores spans (offsets) rather than copied strings
// This avoids string allocations during layout computation
struct WrappedLine {
  std::size_t source_row = 0;     // Which line in the buffer
  std::size_t start_column = 0;   // Start offset within that line
  std::size_t length = 0;         // Character count for this wrapped segment
  
  // For backwards compatibility, we still store text
  // TODO: Phase 2 - remove this and use string_view at render time
  std::string text;
};

// SoA-style layout result: parallel arrays for better cache locality
struct LayoutResult {
  std::vector<std::size_t> source_rows;     // SoA: which buffer line
  std::vector<std::size_t> start_columns;   // SoA: start offset in line
  std::vector<std::size_t> lengths;         // SoA: segment length
  
  std::size_t size() const { return source_rows.size(); }
  bool empty() const { return source_rows.empty(); }
  
  void clear() {
    source_rows.clear();
    start_columns.clear();
    lengths.clear();
  }
  
  void reserve(std::size_t n) {
    source_rows.reserve(n);
    start_columns.reserve(n);
    lengths.reserve(n);
  }
  
  void push_back(std::size_t row, std::size_t col, std::size_t len) {
    source_rows.push_back(row);
    start_columns.push_back(col);
    lengths.push_back(len);
  }
};

// Original API for backwards compatibility
std::vector<WrappedLine> layoutWrappedLines(const TextBuffer &buffer,
                                            std::size_t max_columns);

// New SoA API - avoids string copies, better for large documents
LayoutResult layoutWrappedLinesSoA(const TextBuffer &buffer,
                                   std::size_t max_columns);
