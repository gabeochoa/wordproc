#include "document_io.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "table.h"

// Helper to convert PageMode to string
static std::string pageModeToString(PageMode mode) {
    switch (mode) {
        case PageMode::Paged:
            return "paged";
        case PageMode::Pageless:
        default:
            return "pageless";
    }
}

// Helper to parse PageMode from string
static PageMode pageModeFromString(const std::string &str) {
    if (str == "paged") return PageMode::Paged;
    return PageMode::Pageless;
}

// Helper to convert BorderStyle to string
static std::string borderStyleToString(BorderStyle style) {
    switch (style) {
        case BorderStyle::None: return "none";
        case BorderStyle::Thin: return "thin";
        case BorderStyle::Medium: return "medium";
        case BorderStyle::Thick: return "thick";
        case BorderStyle::Double: return "double";
        case BorderStyle::Dashed: return "dashed";
        case BorderStyle::Dotted: return "dotted";
        default: return "thin";
    }
}

// Helper to parse BorderStyle from string
static BorderStyle borderStyleFromString(const std::string &str) {
    if (str == "none") return BorderStyle::None;
    if (str == "medium") return BorderStyle::Medium;
    if (str == "thick") return BorderStyle::Thick;
    if (str == "double") return BorderStyle::Double;
    if (str == "dashed") return BorderStyle::Dashed;
    if (str == "dotted") return BorderStyle::Dotted;
    return BorderStyle::Thin;
}

// Helper to convert CellAlignment to string
static std::string cellAlignmentToString(CellAlignment align) {
    switch (align) {
        case CellAlignment::TopLeft: return "top-left";
        case CellAlignment::TopCenter: return "top-center";
        case CellAlignment::TopRight: return "top-right";
        case CellAlignment::MiddleLeft: return "middle-left";
        case CellAlignment::MiddleCenter: return "middle-center";
        case CellAlignment::MiddleRight: return "middle-right";
        case CellAlignment::BottomLeft: return "bottom-left";
        case CellAlignment::BottomCenter: return "bottom-center";
        case CellAlignment::BottomRight: return "bottom-right";
        default: return "top-left";
    }
}

// Helper to parse CellAlignment from string
static CellAlignment cellAlignmentFromString(const std::string &str) {
    if (str == "top-center") return CellAlignment::TopCenter;
    if (str == "top-right") return CellAlignment::TopRight;
    if (str == "middle-left") return CellAlignment::MiddleLeft;
    if (str == "middle-center") return CellAlignment::MiddleCenter;
    if (str == "middle-right") return CellAlignment::MiddleRight;
    if (str == "bottom-left") return CellAlignment::BottomLeft;
    if (str == "bottom-center") return CellAlignment::BottomCenter;
    if (str == "bottom-right") return CellAlignment::BottomRight;
    return CellAlignment::TopLeft;
}

// Helper to serialize a table cell
static nlohmann::json serializeCell(const TableCell &cell) {
    nlohmann::json j;
    j["content"] = cell.content;
    j["rowSpan"] = cell.span.rowSpan;
    j["colSpan"] = cell.span.colSpan;
    j["alignment"] = cellAlignmentToString(cell.alignment);
    j["bgColor"] = {
        {"r", cell.backgroundColor.r},
        {"g", cell.backgroundColor.g},
        {"b", cell.backgroundColor.b},
        {"a", cell.backgroundColor.a}
    };
    j["textStyle"] = {
        {"bold", cell.textStyle.bold},
        {"italic", cell.textStyle.italic},
        {"underline", cell.textStyle.underline},
        {"fontSize", cell.textStyle.fontSize}
    };
    j["isMerged"] = cell.isMerged;
    if (cell.isMerged) {
        j["mergeParent"] = {{"row", cell.mergeParent.row}, {"col", cell.mergeParent.col}};
    }
    return j;
}

// Helper to deserialize a table cell
static TableCell deserializeCell(const nlohmann::json &j) {
    TableCell cell;
    if (j.contains("content")) cell.content = j["content"].get<std::string>();
    if (j.contains("rowSpan")) cell.span.rowSpan = j["rowSpan"].get<std::size_t>();
    if (j.contains("colSpan")) cell.span.colSpan = j["colSpan"].get<std::size_t>();
    if (j.contains("alignment")) cell.alignment = cellAlignmentFromString(j["alignment"].get<std::string>());
    if (j.contains("bgColor")) {
        const auto &c = j["bgColor"];
        if (c.contains("r")) cell.backgroundColor.r = c["r"].get<unsigned char>();
        if (c.contains("g")) cell.backgroundColor.g = c["g"].get<unsigned char>();
        if (c.contains("b")) cell.backgroundColor.b = c["b"].get<unsigned char>();
        if (c.contains("a")) cell.backgroundColor.a = c["a"].get<unsigned char>();
    }
    if (j.contains("textStyle")) {
        const auto &ts = j["textStyle"];
        if (ts.contains("bold")) cell.textStyle.bold = ts["bold"].get<bool>();
        if (ts.contains("italic")) cell.textStyle.italic = ts["italic"].get<bool>();
        if (ts.contains("underline")) cell.textStyle.underline = ts["underline"].get<bool>();
        if (ts.contains("fontSize")) cell.textStyle.fontSize = ts["fontSize"].get<int>();
    }
    if (j.contains("isMerged")) cell.isMerged = j["isMerged"].get<bool>();
    if (j.contains("mergeParent") && cell.isMerged) {
        cell.mergeParent.row = j["mergeParent"]["row"].get<std::size_t>();
        cell.mergeParent.col = j["mergeParent"]["col"].get<std::size_t>();
    }
    return cell;
}

// Helper to serialize a table
static nlohmann::json serializeTable(const Table &table) {
    nlohmann::json j;
    j["rows"] = table.rowCount();
    j["cols"] = table.colCount();
    
    // Column widths
    nlohmann::json colWidths = nlohmann::json::array();
    for (std::size_t c = 0; c < table.colCount(); ++c) {
        colWidths.push_back(table.colWidth(c));
    }
    j["colWidths"] = colWidths;
    
    // Row heights
    nlohmann::json rowHeights = nlohmann::json::array();
    for (std::size_t r = 0; r < table.rowCount(); ++r) {
        rowHeights.push_back(table.rowHeight(r));
    }
    j["rowHeights"] = rowHeights;
    
    // Cells
    nlohmann::json cells = nlohmann::json::array();
    for (std::size_t r = 0; r < table.rowCount(); ++r) {
        nlohmann::json row = nlohmann::json::array();
        for (std::size_t c = 0; c < table.colCount(); ++c) {
            row.push_back(serializeCell(table.cell(r, c)));
        }
        cells.push_back(row);
    }
    j["cells"] = cells;
    
    return j;
}

// Helper to deserialize a table
static Table deserializeTable(const nlohmann::json &j) {
    std::size_t rows = j.value("rows", 1);
    std::size_t cols = j.value("cols", 1);
    Table table(rows, cols);
    
    // Column widths
    if (j.contains("colWidths")) {
        const auto &cw = j["colWidths"];
        for (std::size_t c = 0; c < std::min(cols, cw.size()); ++c) {
            table.setColWidth(c, cw[c].get<float>());
        }
    }
    
    // Row heights
    if (j.contains("rowHeights")) {
        const auto &rh = j["rowHeights"];
        for (std::size_t r = 0; r < std::min(rows, rh.size()); ++r) {
            table.setRowHeight(r, rh[r].get<float>());
        }
    }
    
    // Cells
    if (j.contains("cells")) {
        const auto &cells = j["cells"];
        for (std::size_t r = 0; r < std::min(rows, cells.size()); ++r) {
            const auto &row = cells[r];
            for (std::size_t c = 0; c < std::min(cols, row.size()); ++c) {
                TableCell cell = deserializeCell(row[c]);
                table.cell(r, c) = cell;
            }
        }
    }
    
    return table;
}

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
        {"underline", style.underline},
        {"strikethrough", style.strikethrough},
        {"font", style.font},
        {"fontSize", style.fontSize},
        {"textColor", {{"r", style.textColor.r}, {"g", style.textColor.g},
                       {"b", style.textColor.b}, {"a", style.textColor.a}}},
        {"highlightColor", {{"r", style.highlightColor.r}, {"g", style.highlightColor.g},
                            {"b", style.highlightColor.b}, {"a", style.highlightColor.a}}},
    };

    // Page layout settings (document-specific, saved with file)
    doc["pageLayout"] = {
        {"mode", pageModeToString(page.mode)},
        {"pageWidth", page.pageWidth},
        {"pageHeight", page.pageHeight},
        {"pageMargin", page.pageMargin},
        {"lineWidthLimit", page.lineWidthLimit},
    };

    // Font requirements - which fonts and scripts the document needs
    // This enables lazy loading of CJK fonts only when needed
    if (!settings.fontRequirements.empty()) {
        nlohmann::json fonts_array = nlohmann::json::array();
        for (const auto &req : settings.fontRequirements) {
            nlohmann::json font_obj;
            font_obj["fontId"] = req.fontId;
            nlohmann::json scripts_array = nlohmann::json::array();
            for (const auto &script : req.scripts) {
                scripts_array.push_back(scriptRequirementId(script));
            }
            font_obj["scripts"] = scripts_array;
            fonts_array.push_back(font_obj);
        }
        doc["fontRequirements"] = fonts_array;
    }

    ofs << doc.dump(2);
    if (!ofs.good()) {
        result.error = "Failed to write to file: " + path;
        return result;
    }

    result.success = true;
    return result;
}

DocumentResult loadTextFileEx(TextBuffer &buffer, const std::string &path) {
    // Use the new function with default settings (discards loaded settings)
    DocumentSettings settings;
    return loadDocumentEx(buffer, settings, path);
}

DocumentResult loadDocumentEx(TextBuffer &buffer, DocumentSettings &settings,
                              const std::string &path) {
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
            if (version != DocumentSettings::VERSION) {
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

        // Load text style (document-specific settings)
        if (doc.contains("style")) {
            TextStyle &style = settings.textStyle;
            const nlohmann::json &style_json = doc.at("style");
            if (style_json.contains("bold")) {
                style.bold = style_json.at("bold").get<bool>();
            }
            if (style_json.contains("italic")) {
                style.italic = style_json.at("italic").get<bool>();
            }
            if (style_json.contains("underline")) {
                style.underline = style_json.at("underline").get<bool>();
            }
            if (style_json.contains("strikethrough")) {
                style.strikethrough = style_json.at("strikethrough").get<bool>();
            }
            if (style_json.contains("font")) {
                style.font = style_json.at("font").get<std::string>();
            }
            if (style_json.contains("fontSize")) {
                int fontSize = style_json.at("fontSize").get<int>();
                // Clamp to valid range
                style.fontSize = std::max(8, std::min(72, fontSize));
            }
            if (style_json.contains("textColor")) {
                const nlohmann::json &color = style_json.at("textColor");
                if (color.contains("r")) style.textColor.r = color.at("r").get<unsigned char>();
                if (color.contains("g")) style.textColor.g = color.at("g").get<unsigned char>();
                if (color.contains("b")) style.textColor.b = color.at("b").get<unsigned char>();
                if (color.contains("a")) style.textColor.a = color.at("a").get<unsigned char>();
            }
            if (style_json.contains("highlightColor")) {
                const nlohmann::json &color = style_json.at("highlightColor");
                if (color.contains("r")) style.highlightColor.r = color.at("r").get<unsigned char>();
                if (color.contains("g")) style.highlightColor.g = color.at("g").get<unsigned char>();
                if (color.contains("b")) style.highlightColor.b = color.at("b").get<unsigned char>();
                if (color.contains("a")) style.highlightColor.a = color.at("a").get<unsigned char>();
            }
            buffer.setTextStyle(style);
        }

        // Load page layout settings (document-specific settings)
        if (doc.contains("pageLayout")) {
            PageSettings &page = settings.pageSettings;
            const nlohmann::json &page_json = doc.at("pageLayout");
            if (page_json.contains("mode")) {
                page.mode =
                    pageModeFromString(page_json.at("mode").get<std::string>());
            }
            if (page_json.contains("pageWidth")) {
                page.pageWidth = page_json.at("pageWidth").get<float>();
            }
            if (page_json.contains("pageHeight")) {
                page.pageHeight = page_json.at("pageHeight").get<float>();
            }
            if (page_json.contains("pageMargin")) {
                page.pageMargin = page_json.at("pageMargin").get<float>();
            }
            if (page_json.contains("lineWidthLimit")) {
                page.lineWidthLimit =
                    page_json.at("lineWidthLimit").get<float>();
            }
        }

        // Load font requirements (for lazy CJK font loading)
        if (doc.contains("fontRequirements")) {
            settings.fontRequirements.clear();
            for (const auto &font_json : doc.at("fontRequirements")) {
                FontRequirement req;
                if (font_json.contains("fontId")) {
                    req.fontId = font_json.at("fontId").get<std::string>();
                }
                if (font_json.contains("scripts")) {
                    for (const auto &script_str : font_json.at("scripts")) {
                        req.scripts.push_back(
                            parseScriptRequirement(script_str.get<std::string>()));
                    }
                }
                settings.fontRequirements.push_back(req);
            }
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
