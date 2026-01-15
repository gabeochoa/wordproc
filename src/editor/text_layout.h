#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "text_buffer.h"

struct WrappedLine {
  std::size_t source_row = 0;
  std::size_t start_column = 0;
  std::size_t length = 0;
  std::string text;
};

std::vector<WrappedLine> layoutWrappedLines(const TextBuffer &buffer,
                                            std::size_t max_columns);
