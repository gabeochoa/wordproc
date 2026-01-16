#pragma once

#include "../editor/text_buffer.h"
#include "../input/action_map.h"
#include "../ui/win95_widgets.h"
#include <string>
#include <vector>

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
  int offset = 0;       // Scroll offset in lines
  int visibleLines = 20; // Number of visible lines
  int maxScroll = 0;    // Maximum scroll value
};

// Component for document state
struct DocumentComponent : public afterhours::BaseComponent {
  TextBuffer buffer;
  std::string filePath;
  bool isDirty = false;
  
  // For default doc path when saving without a name
  std::string defaultPath = "output/document.wpdoc";
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
  bool showAboutDialog = false;
  bool showHelpWindow = false;  // Keybindings help window
  int helpScrollOffset = 0;     // Scroll position in help window
};

// Page display mode for document layout
enum class PageMode {
  Pageless,  // Continuous flow, no page breaks/margins
  Paged      // Traditional page layout with margins and page breaks
};

// Component for window layout calculations (logic in component_helpers.h)
struct LayoutComponent : public afterhours::BaseComponent {
  float titleBarHeight = 20.0f;
  float menuBarHeight = 20.0f;
  float statusBarHeight = 18.0f;
  float borderWidth = 2.0f;
  float textPadding = 4.0f;
  
  // Page mode settings
  PageMode pageMode = PageMode::Pageless;  // Default to pageless continuous flow
  float pageWidth = 612.0f;    // Letter size in points (8.5" x 72)
  float pageHeight = 792.0f;   // Letter size in points (11" x 72)
  float pageMargin = 72.0f;    // 1 inch margins
  float lineWidthLimit = 0.0f; // 0 = no limit, otherwise max chars per line in pageless mode
  int linesPerPage = 50;       // Approximate lines per page for paged mode
  
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
};

// Component for test mode configuration
struct TestConfigComponent : public afterhours::BaseComponent {
  bool enabled = false;
  std::string screenshotDir = "output/screenshots";
  int frameLimit = 0;
  int frameCount = 0;
};

// Component for input handling (stores the action map for remappable shortcuts)
struct InputComponent : public afterhours::BaseComponent {
  input::ActionMap actionMap;
  
  InputComponent() : actionMap(input::createDefaultActionMap()) {}
};

} // namespace ecs
