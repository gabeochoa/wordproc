#include "document_io.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

bool saveTextFile(const TextBuffer &buffer, const std::string &path) {
    auto result = saveTextFileEx(buffer, path);
    return result.success;
}

bool loadTextFile(TextBuffer &buffer, const std::string &path) {
    auto result = loadTextFileEx(buffer, path);
    return result.success;
}

DocumentResult saveTextFileEx(const TextBuffer &buffer,
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
                result.error =
                    "Unsupported document version: " + std::to_string(version);
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
        result.error =
            "Loaded as plain text (JSON parse error: " + std::string(e.what()) +
            ")";
    }

    return result;
}
