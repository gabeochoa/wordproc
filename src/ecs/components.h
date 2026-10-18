#pragma once

#include <string>
#include <vector>

#include "../editor/autocorrect.h"
#include "../editor/document_settings.h"
#include "../editor/spell_squiggles.h"
#include "../editor/spellcheck.h"
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
    int secondaryOffset = 0;  // Secondary scroll offset for split view
    // Sub-line wheel remainder: fractional lines carried across frames so
    // small wheel deltas accumulate instead of being truncated away.
    // Owned by NavigationSystem via scroll_math::accumulateWheel.
    float wheelRemainder = 0.0f;
    // Scrollbar interaction state (owned by the scrollbar system).
    bool scrollbarDragging = false;
    float scrollbarDragStartY = 0.0f;
    float scrollbarDragStartThumbY = 0.0f;
    int scrollbarDragStartOffset = 0;
    int scrollbarRepeatDir = 0;  // -1 up, +1 down, 0 idle (arrow hold)
    double scrollbarRepeatNext = 0.0;  // get_time() when the next repeat fires
};

// Component for document state
struct DocumentComponent : public afterhours::BaseComponent {
    TextBuffer buffer;
    std::string filePath;
    bool isDirty = false;

    // Document settings (saved with the document file, not with app settings)
    // This includes text style, page mode, margins, etc.
    DocumentSettings docSettings;

    // Comments and revisions
    std::vector<Comment> comments;
    std::vector<Revision> revisions;
    bool trackChangesEnabled = false;
    std::string trackChangesBaseline;

    // Auto-save state
    bool autoSaveEnabled = true;
    double lastAutoSaveTime = 0.0;
    double autoSaveIntervalSeconds = 60.0;
    std::string autoSavePath = "output/autosave.wpdoc";

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
    
    // Spell-check-as-you-type state (session-wide, not saved with the doc).
    // The checker holds the user dictionary + ignore list; the cache holds
    // the visible-lines-only squiggles, rebuilt when the buffer changes.
    SpellChecker spellChecker;
    SpellSquiggleCache spellSquiggles;

    // AutoCorrect engine + toggles (Tools > AutoCorrect...).
    AutoCorrect autoCorrect;

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

// Component for menu bar state (dropdown menus, click dispatch, pending native dialogs)
struct MenuComponent : public afterhours::BaseComponent {
    std::vector<win95::Menu> menus;
    int activeMenuIndex = -1;
    int lastClickedResult = -1;
    int recentFilesCount = 0;
    // Label of the Window menu's current-document item ("1 <name>"),
    // tracked so MenuUISystem rebuilds the bar when the file changes.
    std::string windowDocLabel;

    int consumeClickedResult() {
        int result = lastClickedResult;
        lastClickedResult = -1;
        return result;
    }

    enum class PendingDialog { None, Open, SaveAs };
    PendingDialog pendingDialog = PendingDialog::None;
};

// Component for all dialog visibility and state (separated from menu bar)
struct DialogState : public afterhours::BaseComponent {
    // Quit flow: save-changes prompt (Yes/No/Cancel) and the terminal
    // quit flag the main loop turns into request_quit.
    bool showQuitPrompt = false;
    bool quitRequested = false;
    // Yes on the prompt with no file path opens Save As; quit once it saves.
    bool quitAfterSave = false;

    // About / Help
    bool showAboutDialog = false;
    bool showHelpWindow = false;
    int helpScrollOffset = 0;

    // Word Count
    bool showWordCountDialog = false;

    // Spelling dialog (Tools > Spelling..., F7)
    bool showSpellingDialog = false;
    std::size_t spellSearchOffset = 0;  // Resume scanning from here
    std::size_t spellCurrentOffset = 0;
    std::size_t spellCurrentLength = 0;
    std::string spellCurrentWord;
    std::vector<std::string> spellSuggestions;
    std::string spellReplacementStr;
    bool spellNoErrors = false;  // "Spell check complete"

    // AutoCorrect dialog (Tools > AutoCorrect...)
    bool showAutoCorrectDialog = false;
    // Selected row in the dialog's Replace/With list, as an index into
    // AutoCorrect::entries(); -1 when nothing is selected. The
    // Replace:/With: fields mirror the selected row.
    int autoCorrectSelected = -1;
    std::string autoCorrectReplaceStr;
    std::string autoCorrectWithStr;

    // Find / Replace
    bool showFindDialog = false;
    bool findReplaceMode = false;
    std::string lastSearchTerm;
    std::string replaceTerm;
    FindOptions findOptions;
    char findInputBuffer[256] = {0};
    char replaceInputBuffer[256] = {0};
    std::string findInputStr;
    std::string replaceInputStr;

    // Comment
    bool showCommentDialog = false;
    std::size_t pendingCommentStart = 0;
    std::size_t pendingCommentEnd = 0;
    char commentInputBuffer[256] = {0};
    std::string commentInputStr;

    // Template
    bool showTemplateDialog = false;
    char templateInputBuffer[128] = {0};
    std::string templateInputStr;

    // Tab Width
    bool showTabWidthDialog = false;
    char tabWidthInputBuffer[16] = {0};
    std::string tabWidthInputStr;

    // Page Setup
    bool showPageSetup = false;
    PageSize selectedPageSize = PageSize::Letter;
    PageOrientation selectedOrientation = PageOrientation::Portrait;
    int marginTopMm = 25;
    int marginBottomMm = 25;
    int marginLeftMm = 25;
    int marginRightMm = 25;

    // Save As
    bool showSaveAsDialog = false;
    char saveAsInputBuffer[256] = {0};
    std::string saveAsInputStr;

    // Bookmark List
    bool showBookmarkListDialog = false;

    // Settings
    bool showSettingsDialog = false;
    std::string uiScaleInputStr;

    // Context Menu (right-click)
    bool showContextMenu = false;
    float contextMenuX = 0;
    float contextMenuY = 0;
};

// PageMode is defined in document_settings.h

// Component for window layout calculations (logic in component_helpers.h)
// Note: Page mode settings here mirror
// DocumentComponent.docSettings.pageSettings for display They are synced from
// DocumentSettings on load and saved back on save
struct LayoutComponent : public afterhours::BaseComponent {
    float titleBarHeight = 24.0f;   // Base size, will be scaled in updateLayout
    float menuBarHeight = 20.0f;
    float toolbarHeight = 28.0f;        // Standard toolbar with icon buttons
    float formattingBarHeight = 28.0f;  // Formatting toolbar with style/font dropdowns
    float rulerHeight = 20.0f;          // Horizontal ruler
    float statusBarHeight = 20.0f;
    float borderWidth = 3.0f;
    float textPadding = 8.0f;

    // View options
    float zoomLevel = 1.0f;
    bool focusMode = false;
    bool showRuler = true;  // View > Ruler; hidden reclaims rulerHeight
    bool splitViewEnabled = false;
    bool splitViewHorizontal = true;

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
    
    // Outline panel
    bool showOutline = false;        // Toggle outline panel display
    float outlinePanelWidth = 200.0f;  // Width of outline panel in pixels
};

// Component for test mode configuration
struct TestConfigComponent : public afterhours::BaseComponent {
    bool enabled = false;
    std::string screenshotDir = "tests/screenshots";
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

// Component for toolbar state
// Toolbar icon types for overlay rendering
enum class ToolbarIcon {
    None, New, Open, Save, Print,
    Cut, Copy, Paste, Undo, Redo,
    AlignLeft, AlignCenter, AlignRight, AlignJustify
};

struct ToolbarComponent : public afterhours::BaseComponent {
    // Current selections in dropdowns
    std::string currentStyle = "Normal";
    std::string currentFont = "Times New Roman";
    int currentFontSize = 10;
    
    // Formatting button states (track what's active based on current selection)
    bool boldActive = false;
    bool italicActive = false;
    bool underlineActive = false;
    
    // Alignment button states
    int currentAlignment = 0;  // 0=left, 1=center, 2=right, 3=justify
    
    // Available options for dropdowns
    std::vector<std::string> styles = {"Normal", "Heading 1", "Heading 2", "Heading 3", "Title"};
    std::vector<std::string> fonts = {"Times New Roman", "Arial", "Courier New", "Gaegu", "Gaegu-Bold"};
    std::vector<int> fontSizes = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
    
    // Dropdown open states
    bool styleDropdownOpen = false;
    bool fontDropdownOpen = false;
    bool fontSizeDropdownOpen = false;

};

}  // namespace ecs
