#include "document_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

bool saveTextFile(const TextBuffer &buffer, const std::string &path) {
  std::filesystem::path output_path(path);
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }

  std::ofstream ofs(output_path);
  if (!ofs.is_open()) {
    return false;
  }

  TextStyle style = buffer.textStyle();
  nlohmann::json doc;
  doc["version"] = 1;
  doc["text"] = buffer.getText();
  doc["style"] = {
      {"bold", style.bold},
      {"italic", style.italic},
      {"font", style.font},
      {"fontSize", style.fontSize},
  };

  ofs << doc.dump(2);
  return ofs.good();
}

bool loadTextFile(TextBuffer &buffer, const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return false;
  }

  std::ostringstream contents;
  contents << ifs.rdbuf();
  std::string raw = contents.str();

  try {
    nlohmann::json doc = nlohmann::json::parse(raw);
    if (doc.contains("text")) {
      buffer.setText(doc.at("text").get<std::string>());
    } else {
      buffer.setText(raw);
    }

    if (doc.contains("style")) {
      TextStyle style = buffer.textStyle();
      const nlohmann::json &style_json = doc.at("style");
      if (style_json.contains("bold")) {
        style.bold = style_json.at("bold").get<bool>();
      }
      if (style_json.contains("italic")) {
        style.italic = style_json.at("italic").get<bool>();
      }
      if (style_json.contains("font")) {
        style.font = style_json.at("font").get<std::string>();
      }
      if (style_json.contains("fontSize")) {
        style.fontSize = style_json.at("fontSize").get<int>();
      }
      buffer.setTextStyle(style);
    }
  } catch (...) {
    buffer.setText(raw);
  }

  return true;
}
