#include "document_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

bool saveTextFile(const TextBuffer &buffer, const std::string &path) {
  std::filesystem::path output_path(path);
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }

  std::ofstream ofs(output_path);
  if (!ofs.is_open()) {
    return false;
  }
  ofs << buffer.getText();
  return ofs.good();
}

bool loadTextFile(TextBuffer &buffer, const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return false;
  }

  std::ostringstream contents;
  contents << ifs.rdbuf();
  buffer.setText(contents.str());
  return true;
}
