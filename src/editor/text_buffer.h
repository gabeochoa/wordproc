#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct CaretPosition {
  std::size_t row = 0;
  std::size_t column = 0;
};

struct TextStyle {
  bool bold = false;
  bool italic = false;
  std::string font = "Gaegu-Bold";
  int fontSize = 16;  // Default size in pixels
};

// Line metadata for SoA layout - stores offset and length instead of copying strings
struct LineSpan {
  std::size_t offset = 0;  // Start offset in the character buffer
  std::size_t length = 0;  // Length of line (excluding newline)
};

// Gap buffer for efficient text editing
// Stores characters contiguously with a "gap" at the edit position
// This allows O(1) inserts and deletes at the cursor
class GapBuffer {
public:
  GapBuffer(std::size_t initial_capacity = 4096);
  
  void insert(std::size_t pos, char ch);
  void insertString(std::size_t pos, const char* str, std::size_t len);
  void erase(std::size_t pos, std::size_t count = 1);
  
  char at(std::size_t pos) const;
  std::size_t size() const;
  bool empty() const;
  
  // Get substring without allocating (returns pointer and length)
  const char* data(std::size_t pos, std::size_t len) const;
  
  // Copy substring to output
  void copyTo(std::size_t pos, std::size_t len, char* out) const;
  
  // Get entire buffer as string (for compatibility)
  std::string toString() const;
  
  void clear();
  
private:
  void moveGapTo(std::size_t pos);
  void ensureCapacity(std::size_t needed);
  
  std::vector<char> buffer_;
  std::size_t gap_start_ = 0;
  std::size_t gap_end_ = 0;
};

// SoA (Structure of Arrays) text buffer using gap buffer + line spans
class TextBuffer {
public:
  TextBuffer();

  // Read-only access to lines (returns span count for iteration)
  std::size_t lineCount() const;
  
  // Get line content as string_view-like access (offset + length)
  LineSpan lineSpan(std::size_t row) const;
  
  // Get line content as string (for compatibility - involves copy)
  std::string lineString(std::size_t row) const;
  
  // Legacy API - returns vector of strings (slower, allocates)
  // Kept for compatibility with existing tests and rendering
  std::vector<std::string> lines() const;

  CaretPosition caret() const;
  bool hasSelection() const;
  CaretPosition selectionStart() const;
  CaretPosition selectionEnd() const;

  void setCaret(CaretPosition caret);
  void clearSelection();
  void setSelectionAnchor(CaretPosition anchor);
  void updateSelectionToCaret();

  void insertChar(char ch);
  void insertText(const std::string &text);
  void setText(const std::string &text);
  std::string getText() const;
  TextStyle textStyle() const;
  void setTextStyle(const TextStyle &style);
  void backspace();
  void del();

  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();
  
  // Performance metrics
  struct PerfStats {
    std::size_t total_inserts = 0;
    std::size_t total_deletes = 0;
    std::size_t gap_moves = 0;
    std::size_t buffer_reallocations = 0;
  };
  const PerfStats& perfStats() const { return stats_; }
  void resetPerfStats() { stats_ = {}; }

private:
  void ensureNonEmpty();
  void clampCaret();
  void rebuildLineIndex();
  std::size_t positionToOffset(const CaretPosition& pos) const;
  CaretPosition offsetToPosition(std::size_t offset) const;
  static int comparePositions(const CaretPosition &a,
                              const CaretPosition &b);

  GapBuffer chars_;                   // Contiguous character storage
  std::vector<LineSpan> line_spans_;  // SoA line metadata
  CaretPosition caret_;
  bool has_selection_ = false;
  CaretPosition selection_anchor_;
  CaretPosition selection_end_;
  TextStyle style_;
  PerfStats stats_;
};
