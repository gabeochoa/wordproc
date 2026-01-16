#pragma once

#include <string>

// Document-specific settings saved with the document file
// These are separate from app settings (window size, fullscreen) which
// auto-save immediately

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

// Text styling settings
struct TextStyle {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    std::string font = "Gaegu-Bold";
    int fontSize = 16;  // Default size in pixels
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

    // Document format version (always v0.1 for now per requirements)
    static constexpr int VERSION = 1;
};
