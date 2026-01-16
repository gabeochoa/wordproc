#include "document_io.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

// Helper to convert PageMode to string
static std::string pageModeToString(PageMode mode) {
  switch (mode) {
    case PageMode::Paged: return "paged";
    case PageMode::Pageless: 
    default: return "pageless";
  }
}

// Helper to parse PageMode from string
static PageMode pageModeFromString(const std::string &str) {
  if (str == "paged") return PageMode::Paged;
  return PageMode::Pageless;
}

bool saveTextFile(const TextBuffer &buffer, const std::string &path) {
  auto result = saveTextFileEx(buffer, path);
  return result.success;
}

bool loadTextFile(TextBuffer &buffer, const std::string &path) {
  auto result = loadTextFileEx(buffer, path);
  return result.success;
}

DocumentResult saveTextFileEx(const TextBuffer &buffer, const std::string &path) {
  // Use the new function with default document settings
  DocumentSettings settings;
  settings.textStyle = buffer.textStyle();
  return saveDocumentEx(buffer, settings, path);
}

DocumentResult saveDocumentEx(const TextBuffer &buffer, 
                              const DocumentSettings &settings,
                              const std::string &path) {
  DocumentResult result;
  
  std::filesystem::path output_path(path);
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }

  std::ofstream ofs(output_path);
  if (!ofs.is_open()) {
    result.error = "Could not open file for writing: " + path;
    return result;
  }

  const TextStyle &style = settings.textStyle;
  const PageSettings &page = settings.pageSettings;
  
  nlohmann::json doc;
  doc["version"] = DocumentSettings::VERSION;
  doc["text"] = buffer.getText();
  
  // Style settings (document-specific, saved with file)
  doc["style"] = {
      {"bold", style.bold},
      {"italic", style.italic},
      {"font", style.font},
      {"fontSize", style.fontSize},
  };
  
  // Page layout settings (document-specific, saved with file)
  doc["pageLayout"] = {
      {"mode", pageModeToString(page.mode)},
      {"pageWidth", page.pageWidth},
      {"pageHeight", page.pageHeight},
      {"pageMargin", page.pageMargin},
      {"lineWidthLimit", page.lineWidthLimit},
  };

  ofs << doc.dump(2);
  if (!ofs.good()) {
    result.error = "Failed to write to file: " + path;
    return result;
  }
  
  result.success = true;
  return result;
}

DocumentResult loadTextFileEx(TextBuffer &buffer, const std::string &path) {
  DocumentResult result;
  
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    result.error = "Could not open file: " + path;
    return result;
  }

  std::ostringstream contents;
  contents << ifs.rdbuf();
  std::string raw = contents.str();

  try {
    nlohmann::json doc = nlohmann::json::parse(raw);
    
    // Check version
    if (doc.contains("version")) {
      int version = doc.at("version").get<int>();
      if (version != 1) {
        result.error = "Unsupported document version: " + std::to_string(version);
        result.usedFallback = true;
        // Still try to load
      }
    }
    
    if (doc.contains("text")) {
      buffer.setText(doc.at("text").get<std::string>());
    } else {
      // JSON but no text field - use raw
      buffer.setText(raw);
      result.usedFallback = true;
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
        int fontSize = style_json.at("fontSize").get<int>();
        // Clamp to valid range
        style.fontSize = std::max(8, std::min(72, fontSize));
      }
      buffer.setTextStyle(style);
    }
    
    result.success = true;
  } catch (const std::exception &e) {
    // JSON parse failed - load as plain text
    buffer.setText(raw);
    result.success = true;
    result.usedFallback = true;
    result.error = "Loaded as plain text (JSON parse error: " + std::string(e.what()) + ")";
  }

  return result;
}
