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

// List type for bulleted/numbered paragraphs
enum class ListType {
    None = 0,     // Not a list item
    Bulleted,     // Bullet point (•, ◦, ▪)
    Numbered      // Numbered list (1., 2., 3., or a., b., c., or i., ii., iii.)
};

// Get display name for list type
inline const char* listTypeName(ListType type) {
    switch (type) {
        case ListType::None: return "None";
        case ListType::Bulleted: return "Bulleted";
        case ListType::Numbered: return "Numbered";
        default: return "None";
    }
}

// Get bullet character for a given list level (0-based)
inline const char* bulletForLevel(int level) {
    switch (level % 3) {
        case 0: return "\xE2\x80\xA2";  // • (bullet)
        case 1: return "\xE2\x97\xA6";  // ◦ (white bullet)
        case 2: return "\xE2\x96\xAA";  // ▪ (small square)
        default: return "\xE2\x80\xA2";
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
        case ParagraphStyle::Count:
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
        case ParagraphStyle::Count:
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
        case ParagraphStyle::Normal:
        case ParagraphStyle::Subtitle:
        case ParagraphStyle::Heading5:
        case ParagraphStyle::Heading6:
        case ParagraphStyle::Count:
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

// Hyperlink structure for linking text ranges to URLs
struct Hyperlink {
    std::size_t startOffset = 0;  // Start position in document (character offset)
    std::size_t endOffset = 0;    // End position (exclusive)
    std::string url;              // Target URL (http://, https://, mailto:, file://)
    std::string tooltip;          // Optional tooltip text
    
    // Check if this hyperlink contains a given position
    bool contains(std::size_t pos) const {
        return pos >= startOffset && pos < endOffset;
    }
    
    // Check if this hyperlink overlaps with a range
    bool overlaps(std::size_t start, std::size_t end) const {
        return startOffset < end && endOffset > start;
    }
    
    // Get the length of the hyperlink
    std::size_t length() const {
        return endOffset - startOffset;
    }
    
    bool operator==(const Hyperlink& other) const {
        return startOffset == other.startOffset && 
               endOffset == other.endOffset && 
               url == other.url;
    }
};

// Bookmark structure for internal document navigation
struct Bookmark {
    std::string name;              // Unique name/ID for the bookmark
    std::size_t offset = 0;        // Character offset in document
    std::string displayName;       // Optional user-friendly display name
    
    // Check if bookmark is at a specific position
    bool isAt(std::size_t pos) const {
        return offset == pos;
    }
    
    // Get the name to display (displayName if set, otherwise name)
    std::string getDisplayName() const {
        return displayName.empty() ? name : displayName;
    }
    
    bool operator==(const Bookmark& other) const {
        return name == other.name && offset == other.offset;
    }
    bool operator!=(const Bookmark& other) const { return !(*this == other); }
    bool operator<(const Bookmark& other) const { return offset < other.offset; }
};

// Footnote structure for document footnotes with auto-numbering
struct Footnote {
    std::size_t referenceOffset = 0;  // Position of footnote marker in main text
    std::string content;               // Footnote content/text
    int number = 0;                    // Auto-assigned footnote number (1, 2, 3...)
    
    bool operator==(const Footnote& other) const {
        return referenceOffset == other.referenceOffset && content == other.content;
    }
    bool operator!=(const Footnote& other) const { return !(*this == other); }
    bool operator<(const Footnote& other) const { return referenceOffset < other.referenceOffset; }
};

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

// Page orientation
enum class PageOrientation {
    Portrait,   // Height > Width (default)
    Landscape   // Width > Height
};

// Get display name for page orientation
inline const char* pageOrientationName(PageOrientation orient) {
    switch (orient) {
        case PageOrientation::Portrait: return "Portrait";
        case PageOrientation::Landscape: return "Landscape";
        default: return "Portrait";
    }
}

// Predefined page sizes (dimensions in points, 1 inch = 72 points)
enum class PageSize {
    Letter,     // 8.5" x 11" (US standard)
    Legal,      // 8.5" x 14"
    Tabloid,    // 11" x 17"
    A4,         // 210mm x 297mm
    A5,         // 148mm x 210mm
    B5,         // 176mm x 250mm
    Executive,  // 7.25" x 10.5"
    Custom      // User-defined dimensions
};

// Get display name for page size
inline const char* pageSizeName(PageSize size) {
    switch (size) {
        case PageSize::Letter: return "Letter (8.5\" x 11\")";
        case PageSize::Legal: return "Legal (8.5\" x 14\")";
        case PageSize::Tabloid: return "Tabloid (11\" x 17\")";
        case PageSize::A4: return "A4 (210mm x 297mm)";
        case PageSize::A5: return "A5 (148mm x 210mm)";
        case PageSize::B5: return "B5 (176mm x 250mm)";
        case PageSize::Executive: return "Executive (7.25\" x 10.5\")";
        case PageSize::Custom: return "Custom";
        default: return "Letter";
    }
}

// Get page dimensions in points for a page size (portrait orientation)
inline void getPageDimensions(PageSize size, float& width, float& height) {
    switch (size) {
        case PageSize::Letter:
            width = 612.0f;   // 8.5" * 72
            height = 792.0f;  // 11" * 72
            break;
        case PageSize::Legal:
            width = 612.0f;   // 8.5" * 72
            height = 1008.0f; // 14" * 72
            break;
        case PageSize::Tabloid:
            width = 792.0f;   // 11" * 72
            height = 1224.0f; // 17" * 72
            break;
        case PageSize::A4:
            width = 595.0f;   // 210mm * 72/25.4
            height = 842.0f;  // 297mm * 72/25.4
            break;
        case PageSize::A5:
            width = 420.0f;   // 148mm * 72/25.4
            height = 595.0f;  // 210mm * 72/25.4
            break;
        case PageSize::B5:
            width = 499.0f;   // 176mm * 72/25.4
            height = 709.0f;  // 250mm * 72/25.4
            break;
        case PageSize::Executive:
            width = 522.0f;   // 7.25" * 72
            height = 756.0f;  // 10.5" * 72
            break;
        case PageSize::Custom:
        default:
            width = 612.0f;
            height = 792.0f;
            break;
    }
}

// Section break type
enum class SectionBreakType {
    NextPage,      // New section starts on next page
    Continuous,    // New section continues on same page
    EvenPage,      // New section starts on next even-numbered page
    OddPage        // New section starts on next odd-numbered page
};

// Section configuration (each document can have multiple sections)
// Sections allow different page settings within the same document
struct SectionSettings {
    // Layout settings for this section
    float pageWidth = 612.0f;
    float pageHeight = 792.0f;
    float marginTop = 72.0f;
    float marginBottom = 72.0f;
    float marginLeft = 72.0f;
    float marginRight = 72.0f;
    PageOrientation orientation = PageOrientation::Portrait;
    
    // Column settings
    int columns = 1;           // Number of columns (1 = single column)
    float columnSpacing = 36.0f;  // Space between columns in points
    
    // Section break type (how this section starts)
    SectionBreakType breakType = SectionBreakType::NextPage;
    
    // Headers/footers can be different per section
    bool linkToPrevious = true;  // Use same header/footer as previous section
    // Note: HeaderFooter and Watermark members moved to after struct definitions
    // to avoid forward declaration issues. For now, use pointers or indices.
    // TODO: Reorganize file to define HeaderFooter/Watermark before SectionSettings
    
    // Starting page number (0 = continue from previous)
    int startingPageNumber = 0;
};

// Section marker in document
struct DocumentSection {
    std::size_t startLine = 0;      // Line where section starts
    SectionSettings settings;        // Settings for this section
    
    bool operator<(const DocumentSection& other) const {
        return startLine < other.startLine;
    }
};

// Page layout settings
struct PageSettings {
    PageMode mode = PageMode::Pageless;
    PageSize size = PageSize::Letter;           // Preset page size
    PageOrientation orientation = PageOrientation::Portrait;
    float pageWidth = 612.0f;   // Letter size in points (8.5" x 72)
    float pageHeight = 792.0f;  // Letter size in points (11" x 72)
    float pageMargin = 72.0f;   // 1 inch margins (all sides)
    float marginTop = 72.0f;    // Top margin in points
    float marginBottom = 72.0f; // Bottom margin in points
    float marginLeft = 72.0f;   // Left margin in points
    float marginRight = 72.0f;  // Right margin in points
    float lineWidthLimit =
        0.0f;  // 0 = no limit, otherwise max chars per line in pageless mode
    TextColor pageColor = TextColors::White;  // Page background color
    
    // Apply a page size preset (updates width/height based on orientation)
    void applyPageSize(PageSize newSize) {
        size = newSize;
        float w, h;
        getPageDimensions(size, w, h);
        if (orientation == PageOrientation::Portrait) {
            pageWidth = w;
            pageHeight = h;
        } else {
            pageWidth = h;
            pageHeight = w;
        }
    }
    
    // Toggle orientation (swaps width/height)
    void toggleOrientation() {
        if (orientation == PageOrientation::Portrait) {
            orientation = PageOrientation::Landscape;
        } else {
            orientation = PageOrientation::Portrait;
        }
        std::swap(pageWidth, pageHeight);
    }
    
    // Set all margins uniformly
    void setUniformMargins(float margin) {
        pageMargin = margin;
        marginTop = margin;
        marginBottom = margin;
        marginLeft = margin;
        marginRight = margin;
    }
};

// Page number format options
enum class PageNumberFormat {
    None,           // No page numbers
    Arabic,         // 1, 2, 3, ...
    RomanLower,     // i, ii, iii, ...
    RomanUpper,     // I, II, III, ...
    LetterLower,    // a, b, c, ...
    LetterUpper     // A, B, C, ...
};

// Page number position within header/footer
enum class PageNumberPosition {
    Left,
    Center,
    Right
};

// Header/footer content section
struct HeaderFooterSection {
    std::string text;           // Static text content
    bool showPageNumber = false;  // Include page number in this section
    PageNumberFormat format = PageNumberFormat::Arabic;
    bool showTotalPages = false;  // Show "Page X of Y" format
};

// Header or footer configuration
struct HeaderFooter {
    bool enabled = false;
    HeaderFooterSection left;    // Left-aligned content
    HeaderFooterSection center;  // Center-aligned content
    HeaderFooterSection right;   // Right-aligned content
    float height = 36.0f;        // Height in points (0.5 inch default)
    TextStyle style;             // Font style for header/footer
    bool differentFirstPage = false;  // Use different header/footer on first page
    bool differentOddEven = false;    // Different headers for odd/even pages
    
    // Get display text for a section including page number
    std::string getSectionText(const HeaderFooterSection& section, 
                               int pageNum, int totalPages) const {
        std::string result = section.text;
        if (section.showPageNumber) {
            std::string pageStr = formatPageNumber(pageNum, section.format);
            if (section.showTotalPages) {
                pageStr += " of " + std::to_string(totalPages);
            }
            if (!result.empty()) result += " ";
            result += pageStr;
        }
        return result;
    }
    
    // Format a page number according to the format setting
    static std::string formatPageNumber(int num, PageNumberFormat format) {
        switch (format) {
            case PageNumberFormat::None: return "";
            case PageNumberFormat::Arabic: return std::to_string(num);
            case PageNumberFormat::RomanLower: return toRomanLower(num);
            case PageNumberFormat::RomanUpper: return toRomanUpper(num);
            case PageNumberFormat::LetterLower: return toLetterLower(num);
            case PageNumberFormat::LetterUpper: return toLetterUpper(num);
            default: return std::to_string(num);
        }
    }
    
    static std::string toRomanLower(int num) {
        std::string upper = toRomanUpper(num);
        std::string lower;
        for (char c : upper) lower += static_cast<char>(std::tolower(c));
        return lower;
    }
    
    static std::string toRomanUpper(int num) {
        if (num <= 0 || num > 3999) return std::to_string(num);
        std::string result;
        const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
        const char* numerals[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
        for (int i = 0; i < 13; ++i) {
            while (num >= values[i]) {
                result += numerals[i];
                num -= values[i];
            }
        }
        return result;
    }
    
    static std::string toLetterLower(int num) {
        if (num <= 0) return std::to_string(num);
        std::string result;
        while (num > 0) {
            result = static_cast<char>('a' + (num - 1) % 26) + result;
            num = (num - 1) / 26;
        }
        return result;
    }
    
    static std::string toLetterUpper(int num) {
        std::string lower = toLetterLower(num);
        std::string upper;
        for (char c : lower) upper += static_cast<char>(std::toupper(c));
        return upper;
    }
};

// Watermark type
enum class WatermarkType {
    None,   // No watermark
    Text,   // Text watermark (e.g., "CONFIDENTIAL", "DRAFT")
    Image   // Image watermark (path to image file)
};

// Watermark configuration for document pages
struct Watermark {
    WatermarkType type = WatermarkType::None;
    std::string text;           // Text content (for Text type)
    std::string imagePath;      // Path to image (for Image type)
    float opacity = 0.3f;       // Transparency (0.0 = invisible, 1.0 = fully opaque)
    float rotation = -45.0f;    // Rotation in degrees (diagonal by default)
    float scale = 1.0f;         // Scale factor for rendering
    TextColor color = {200, 200, 200, 255};  // Light gray by default for text
    std::string font = "Gaegu-Bold";  // Font for text watermark
    int fontSize = 72;          // Font size for text watermark
    
    bool isEnabled() const { return type != WatermarkType::None; }
};

// Combined document settings - saved/loaded with document file
struct DocumentSettings {
    TextStyle textStyle;
    PageSettings pageSettings;
    HeaderFooter header;    // Document header configuration
    HeaderFooter footer;    // Document footer configuration
    Watermark watermark;    // Document watermark configuration

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
