#pragma once

#include <string>

// Document-specific settings saved with the document file
// These are separate from app settings (window size, fullscreen) which
// auto-save immediately

// Text styling settings
struct TextStyle {
    bool bold = false;
    bool italic = false;
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
