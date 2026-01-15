#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct CaretPosition {
  std::size_t row = 0;
  std::size_t column = 0;
};

class TextBuffer {
public:
  TextBuffer();

  const std::vector<std::string> &lines() const;
  CaretPosition caret() const;

  void setCaret(CaretPosition caret);

  void insertChar(char ch);
  void insertText(const std::string &text);
  void backspace();
  void del();

  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();

private:
  void ensureNonEmpty();
  void clampCaret();

  std::vector<std::string> lines_;
  CaretPosition caret_;
};
