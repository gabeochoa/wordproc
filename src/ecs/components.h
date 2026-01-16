#pragma once

#include <string>
#include <vector>

#include "../editor/document_settings.h"
#include "../editor/drawing.h"
#include "../editor/equation.h"
#include "../editor/image.h"
#include "../editor/table.h"
#include "../editor/text_buffer.h"
#include "../input/action_map.h"
#include "../ui/win95_widgets.h"

// Include afterhours base component
#include "../../vendor/afterhours/src/core/base_component.h"

namespace ecs {

// Component for caret blinking state (pure data - logic in component_helpers.h)
struct CaretComponent : public afterhours::BaseComponent {
    double blinkTimer = 0.0;
    bool visible = true;
    static constexpr double BLINK_INTERVAL = 0.5;
};

// Component for scroll state (pure data - logic in component_helpers.h)
struct ScrollComponent : public afterhours::BaseComponent {
    int offset = 0;         // Scroll offset in lines
    int visibleLines = 20;  // Number of visible lines
    int maxScroll = 0;      // Maximum scroll value
};

// Component for document state
struct DocumentComponent : public afterhours::BaseComponent {
    TextBuffer buffer;
    std::string filePath;
    bool isDirty = false;

    // Document settings (saved with the document file, not with app settings)
    // This includes text style, page mode, margins, etc.
    DocumentSettings docSettings;

    // Tables embedded in the document (indexed by position in text)
    // Each table is associated with a line number where it appears
    std::vector<std::pair<std::size_t, Table>> tables;

    // For default doc path when saving without a name
    std::string defaultPath = "output/document.wpdoc";
    
    // Table helper methods
    void insertTable(std::size_t atLine, std::size_t rows, std::size_t cols) {
        tables.emplace_back(atLine, Table(rows, cols));
    }
    
    Table* tableAtLine(std::size_t line) {
        for (auto& [lineNum, table] : tables) {
            if (lineNum == line) return &table;
        }
        return nullptr;
    }
    
    const Table* tableAtLine(std::size_t line) const {
        for (const auto& [lineNum, table] : tables) {
            if (lineNum == line) return &table;
        }
        return nullptr;
    }
    
    void removeTable(std::size_t atLine) {
        tables.erase(
            std::remove_if(tables.begin(), tables.end(),
                [atLine](const auto& p) { return p.first == atLine; }),
            tables.end());
    }
    
    // Images embedded in the document
    ImageCollection images;
    
    // Image helper methods
    void insertImage(const DocumentImage& image) {
        images.addImage(image);
    }
    
    DocumentImage* imageById(std::size_t id) {
        return images.getImage(id);
    }
    
    const DocumentImage* imageById(std::size_t id) const {
        return images.getImage(id);
    }
    
    void removeImage(std::size_t id) {
        images.removeImage(id);
    }
    
    // Drawings/shapes embedded in the document
    DrawingCollection drawings;
    
    // Equations embedded in the document
    EquationCollection equations;
    
    // Drawing helper methods
    void insertDrawing(const DocumentDrawing& drawing) {
        drawings.addDrawing(drawing);
    }
    
    DocumentDrawing* drawingById(std::size_t id) {
        return drawings.getDrawing(id);
    }
    
    const DocumentDrawing* drawingById(std::size_t id) const {
        return drawings.getDrawing(id);
    }
    
    void removeDrawing(std::size_t id) {
        drawings.removeDrawing(id);
    }
};

// Component for table editing state
struct TableEditComponent : public afterhours::BaseComponent {
    bool isEditingTable = false;
    std::size_t editingTableLine = 0;  // Line number of the table being edited
    CellPosition currentCell{0, 0};     // Current cell being edited
    bool hasSelection = false;
    CellPosition selectionStart{0, 0};
    CellPosition selectionEnd{0, 0};
};

// Component for status messages (pure data - logic in component_helpers.h)
struct StatusComponent : public afterhours::BaseComponent {
    std::string text;
    double expiresAt = 0.0;
    bool isError = false;
};

// Component for menu state
struct MenuComponent : public afterhours::BaseComponent {
    std::vector<win95::Menu> menus;
    int activeMenuIndex = -1;      // Currently active menu (-1 = none)
    int lastClickedResult = -1;    // Result of last menu click for action handling
    bool showAboutDialog = false;
    bool showHelpWindow = false;  // Keybindings help window
    int helpScrollOffset = 0;     // Scroll position in help window
    
    // Consume the clicked result (returns it and clears it)
    int consumeClickedResult() {
        int result = lastClickedResult;
        lastClickedResult = -1;
        return result;
    }
    
    // Find/Replace state
    bool showFindDialog = false;
    bool findReplaceMode = false;  // false = find only, true = find + replace
    std::string lastSearchTerm;
    std::string replaceTerm;
    FindOptions findOptions;       // Case sensitive, whole word, wrap around
    char findInputBuffer[256] = {0};
    char replaceInputBuffer[256] = {0};
    
    // Page Setup dialog state
    bool showPageSetup = false;
    PageSize selectedPageSize = PageSize::Letter;
    PageOrientation selectedOrientation = PageOrientation::Portrait;
    int marginTopMm = 25;     // Margins in mm for UI
    int marginBottomMm = 25;
    int marginLeftMm = 25;
    int marginRightMm = 25;
};

// PageMode is defined in document_settings.h

// Component for window layout calculations (logic in component_helpers.h)
// Note: Page mode settings here mirror
// DocumentComponent.docSettings.pageSettings for display They are synced from
// DocumentSettings on load and saved back on save
struct LayoutComponent : public afterhours::BaseComponent {
    float titleBarHeight = 20.0f;
    float menuBarHeight = 20.0f;
    float statusBarHeight = 18.0f;
    float borderWidth = 2.0f;
    float textPadding = 4.0f;

    // Page mode settings
    PageMode pageMode =
        PageMode::Pageless;     // Default to pageless continuous flow
    float pageWidth = 612.0f;   // Letter size in points (8.5" x 72)
    float pageHeight = 792.0f;  // Letter size in points (11" x 72)
    float pageMargin = 72.0f;   // 1 inch margins
    float lineWidthLimit =
        0.0f;  // 0 = no limit, otherwise max chars per line in pageless mode
    int linesPerPage = 50;  // Approximate lines per page for paged mode

    // Computed values (updated each frame based on window size)
    int screenWidth = 800;
    int screenHeight = 600;

    struct Rect {
        float x, y, width, height;
    };

    Rect titleBar{};
    Rect menuBar{};
    Rect statusBar{};
    Rect textArea{};

    // Page-specific computed values
    float pageDisplayWidth = 0.0f;   // Scaled page width for display
    float pageDisplayHeight = 0.0f;  // Scaled page height for display
    float pageScale = 1.0f;          // Scale factor for page display
    float pageOffsetX = 0.0f;        // X offset to center page in window
    
    // Line numbering
    bool showLineNumbers = false;    // Toggle line number display in gutter
    float lineNumberGutterWidth = 50.0f;  // Width of line number gutter in pixels
};

// Component for test mode configuration
struct TestConfigComponent : public afterhours::BaseComponent {
    bool enabled = false;
    std::string screenshotDir = "output/screenshots";
    int frameLimit = 0;
    int frameCount = 0;

    // FPS test mode - simulates scrolling and logs FPS
    bool fpsTestMode = false;
    float fpsSum = 0.0f;
    float fpsMin = 999999.0f;
    float fpsMax = 0.0f;
    int fpsSamples = 0;
    
    // E2E debug overlay - shows current command and timeout
    bool e2eDebugOverlay = false;
    std::string e2eCurrentCommand;
    float e2eTimeoutSeconds = -1.0f;  // -1 = no timeout
};

// Component for input handling (stores the action map for remappable shortcuts)
struct InputComponent : public afterhours::BaseComponent {
    input::ActionMap actionMap;

    InputComponent() : actionMap(input::createDefaultActionMap()) {}
};

}  // namespace ecs
