#pragma once

#include <algorithm>
#include <cctype>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <vector>

// Document-specific settings saved with the document file
// These are separate from app settings (window size, fullscreen) which
// auto-save immediately

// Language/script identifiers for lazy font loading
// When a document uses CJK text, we record which scripts are needed
// so we can load the appropriate fonts on demand
enum class ScriptRequirement {
    Latin,     // ASCII + Latin Extended (default, always loaded)
    Korean,    // Hangul syllables and jamo
    Japanese,  // Hiragana, Katakana, Kanji
    Chinese,   // Simplified/Traditional Chinese
    Arabic,    // Arabic script
    Hebrew,    // Hebrew script
    Cyrillic,  // Russian, etc.
    Greek,     // Greek alphabet
    Thai,      // Thai script
    Count
};

// Get identifier string for script (used in file format) - lowercase
inline std::string scriptRequirementId(ScriptRequirement script) {
    auto name = std::string(magic_enum::enum_name(script));
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return name;
}

// Parse script requirement from string (case-insensitive)
inline ScriptRequirement parseScriptRequirement(const std::string& id) {
    // Convert to title case for magic_enum lookup
    std::string normalized = id;
    if (!normalized.empty()) {
        normalized[0] = static_cast<char>(std::toupper(normalized[0]));
        for (size_t i = 1; i < normalized.size(); ++i) {
            normalized[i] = static_cast<char>(std::tolower(normalized[i]));
        }
    }
    auto result = magic_enum::enum_cast<ScriptRequirement>(normalized);
    return result.value_or(ScriptRequirement::Latin);
}

// Font requirement for a document
struct FontRequirement {
    std::string fontId;  // Font identifier (e.g., "Gaegu-Bold", "NotoSansKR")
    std::vector<ScriptRequirement> scripts;  // Which scripts this font provides
};

// Paragraph styles for document structure (H1-H6, title, etc.)
enum class ParagraphStyle {
    Normal = 0,   // Regular body text
    Title,        // Document title (largest)
    Subtitle,     // Document subtitle
    Heading1,     // H1 - largest heading
    Heading2,     // H2
    Heading3,     // H3
    Heading4,     // H4
    Heading5,     // H5
    Heading6,     // H6 - smallest heading
    Count         // Number of styles (for iteration)
};

// Text alignment for paragraphs
enum class TextAlignment {
    Left = 0,   // Left-aligned (default)
    Center,     // Centered
    Right,      // Right-aligned
    Justify     // Justified (not yet implemented in rendering)
};

// Get display name for text alignment
inline const char* textAlignmentName(TextAlignment align) {
    switch (align) {
        case TextAlignment::Left: return "Left";
        case TextAlignment::Center: return "Center";
        case TextAlignment::Right: return "Right";
        case TextAlignment::Justify: return "Justify";
        default: return "Left";
    }
}

// Get display name for a paragraph style
inline const char* paragraphStyleName(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Normal: return "Normal";
        case ParagraphStyle::Title: return "Title";
        case ParagraphStyle::Subtitle: return "Subtitle";
        case ParagraphStyle::Heading1: return "Heading 1";
        case ParagraphStyle::Heading2: return "Heading 2";
        case ParagraphStyle::Heading3: return "Heading 3";
        case ParagraphStyle::Heading4: return "Heading 4";
        case ParagraphStyle::Heading5: return "Heading 5";
        case ParagraphStyle::Heading6: return "Heading 6";
        default: return "Normal";
    }
}

// Get font size for a paragraph style (base size is 16)
inline int paragraphStyleFontSize(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title: return 32;
        case ParagraphStyle::Subtitle: return 24;
        case ParagraphStyle::Heading1: return 28;
        case ParagraphStyle::Heading2: return 24;
        case ParagraphStyle::Heading3: return 20;
        case ParagraphStyle::Heading4: return 18;
        case ParagraphStyle::Heading5: return 16;
        case ParagraphStyle::Heading6: return 14;
        case ParagraphStyle::Normal:
        default: return 16;
    }
}

// Check if a paragraph style should be bold
inline bool paragraphStyleIsBold(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title:
        case ParagraphStyle::Heading1:
        case ParagraphStyle::Heading2:
        case ParagraphStyle::Heading3:
        case ParagraphStyle::Heading4:
            return true;
        default:
            return false;
    }
}

// Check if a paragraph style should be italic
inline bool paragraphStyleIsItalic(ParagraphStyle style) {
    return style == ParagraphStyle::Subtitle;
}

// Color represented as RGBA for document storage
struct TextColor {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;
    
    bool operator==(const TextColor& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const TextColor& other) const { return !(*this == other); }
    
    // Check if color is the "none" color (transparent, used for no highlight)
    bool isNone() const { return a == 0; }
};

// Predefined text colors
namespace TextColors {
    constexpr TextColor Black = {0, 0, 0, 255};
    constexpr TextColor White = {255, 255, 255, 255};
    constexpr TextColor Red = {200, 0, 0, 255};
    constexpr TextColor DarkRed = {128, 0, 0, 255};
    constexpr TextColor Orange = {255, 128, 0, 255};
    constexpr TextColor Yellow = {255, 255, 0, 255};
    constexpr TextColor Green = {0, 128, 0, 255};
    constexpr TextColor Blue = {0, 0, 200, 255};
    constexpr TextColor DarkBlue = {0, 0, 128, 255};
    constexpr TextColor Purple = {128, 0, 128, 255};
    constexpr TextColor Gray = {128, 128, 128, 255};
    constexpr TextColor None = {0, 0, 0, 0};  // Transparent (no highlight)
}

// Predefined highlight colors (lighter versions for background)
namespace HighlightColors {
    constexpr TextColor None = {0, 0, 0, 0};  // Transparent (no highlight)
    constexpr TextColor Yellow = {255, 255, 0, 255};
    constexpr TextColor Green = {0, 255, 0, 255};
    constexpr TextColor Cyan = {0, 255, 255, 255};
    constexpr TextColor Pink = {255, 192, 203, 255};
    constexpr TextColor Orange = {255, 200, 100, 255};
    constexpr TextColor Blue = {173, 216, 230, 255};
    constexpr TextColor Purple = {221, 160, 221, 255};
    constexpr TextColor Gray = {211, 211, 211, 255};
}

// Text styling settings
struct TextStyle {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    std::string font = "Gaegu-Bold";
    int fontSize = 16;  // Default size in pixels
    TextColor textColor = TextColors::Black;  // Text color (default black)
    TextColor highlightColor = HighlightColors::None;  // Highlight/background color (default none)
};

// Page layout mode
enum class PageMode {
    Pageless,  // Continuous flow, no page breaks/margins
    Paged      // Traditional page layout with margins and page breaks
};

// Page layout settings
struct PageSettings {
    PageMode mode = PageMode::Pageless;
    float pageWidth = 612.0f;   // Letter size in points (8.5" x 72)
    float pageHeight = 792.0f;  // Letter size in points (11" x 72)
    float pageMargin = 72.0f;   // 1 inch margins
    float lineWidthLimit =
        0.0f;  // 0 = no limit, otherwise max chars per line in pageless mode
};

// Combined document settings - saved/loaded with document file
struct DocumentSettings {
    TextStyle textStyle;
    PageSettings pageSettings;

    // Font requirements - which fonts and scripts the document needs
    // This enables lazy loading of CJK fonts only when needed
    std::vector<FontRequirement> fontRequirements;

    // Helper to check if a script is required by this document
    bool requiresScript(ScriptRequirement script) const {
        for (const auto& req : fontRequirements) {
            for (const auto& s : req.scripts) {
                if (s == script) return true;
            }
        }
        return false;
    }

    // Helper to add a font requirement
    void addFontRequirement(const std::string& fontId,
                            std::vector<ScriptRequirement> scripts) {
        fontRequirements.push_back({fontId, std::move(scripts)});
    }

    // Document format version (always v0.1 for now per requirements)
    static constexpr int VERSION = 1;
};
