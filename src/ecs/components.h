#pragma once

#include "../editor/text_buffer.h"
#include "../ui/win95_widgets.h"
#include <string>
#include <vector>

// Include afterhours base component
#include "../../vendor/afterhours/src/core/base_component.h"

namespace ecs {

// Component for caret blinking state
struct CaretComponent : public afterhours::BaseComponent {
  double blinkTimer = 0.0;
  bool visible = true;
  static constexpr double BLINK_INTERVAL = 0.5;

  void update(float dt) {
    blinkTimer += dt;
    if (blinkTimer >= BLINK_INTERVAL) {
      blinkTimer = 0.0;
      visible = !visible;
    }
  }

  void resetBlink() {
    visible = true;
    blinkTimer = 0.0;
  }
};

// Component for scroll state
struct ScrollComponent : public afterhours::BaseComponent {
  int offset = 0;       // Scroll offset in lines
  int visibleLines = 20; // Number of visible lines
  int maxScroll = 0;    // Maximum scroll value

  void clamp(int lineCount) {
    maxScroll = lineCount - visibleLines;
    if (maxScroll < 0) maxScroll = 0;
    if (offset < 0) offset = 0;
    if (offset > maxScroll) offset = maxScroll;
  }

  // Auto-scroll to keep caret visible
  void scrollToRow(int row) {
    if (row < offset) {
      offset = row;
    } else if (row >= offset + visibleLines) {
      offset = row - visibleLines + 1;
    }
  }
};

// Component for document state
struct DocumentComponent : public afterhours::BaseComponent {
  TextBuffer buffer;
  std::string filePath;
  bool isDirty = false;
  
  // For default doc path when saving without a name
  std::string defaultPath = "output/document.wpdoc";
};

// Component for status messages
struct StatusComponent : public afterhours::BaseComponent {
  std::string text;
  double expiresAt = 0.0;
  bool isError = false;

  void set(const std::string& msg, bool error = false) {
    text = msg;
    isError = error;
    // Note: expiresAt should be set by caller with current time + duration
  }

  bool hasMessage(double currentTime) const {
    return !text.empty() && currentTime < expiresAt;
  }
};

// Component for menu state
struct MenuComponent : public afterhours::BaseComponent {
  std::vector<win95::Menu> menus;
  bool showAboutDialog = false;
};

// Page display mode for document layout
enum class PageMode {
  Pageless,  // Continuous flow, no page breaks/margins
  Paged      // Traditional page layout with margins and page breaks
};

// Component for window layout calculations
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
  
  Rect titleBar;
  Rect menuBar;
  Rect statusBar;
  Rect textArea;
  
  // Page-specific computed values
  float pageDisplayWidth = 0.0f;   // Scaled page width for display
  float pageDisplayHeight = 0.0f;  // Scaled page height for display
  float pageScale = 1.0f;          // Scale factor for page display
  float pageOffsetX = 0.0f;        // X offset to center page in window
  
  void updateLayout(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    
    titleBar = {0, 0, static_cast<float>(w), titleBarHeight};
    menuBar = {0, titleBarHeight, static_cast<float>(w), menuBarHeight};
    statusBar = {0, static_cast<float>(h - statusBarHeight),
                 static_cast<float>(w), statusBarHeight};
    
    float textTop = titleBarHeight + menuBarHeight + borderWidth;
    float textHeight = static_cast<float>(h) - titleBarHeight - menuBarHeight -
                       statusBarHeight - 2 * borderWidth;
    textArea = {borderWidth, textTop,
                static_cast<float>(w) - 2 * borderWidth, textHeight};
    
    // Calculate page display dimensions for paged mode
    if (pageMode == PageMode::Paged) {
      float availableWidth = textArea.width - 20.0f;  // Margin for page shadow
      float availableHeight = textArea.height - 20.0f;
      
      // Scale to fit horizontally
      pageScale = availableWidth / pageWidth;
      if (pageScale * pageHeight > availableHeight * 0.9f) {
        // If page would be too tall, scale to fit vertically
        pageScale = (availableHeight * 0.9f) / pageHeight;
      }
      
      pageDisplayWidth = pageWidth * pageScale;
      pageDisplayHeight = pageHeight * pageScale;
      pageOffsetX = textArea.x + (textArea.width - pageDisplayWidth) / 2.0f;
    }
  }
  
  // Get effective text area (within page margins for paged mode)
  Rect effectiveTextArea() const {
    if (pageMode == PageMode::Pageless) {
      // Apply line width limit if set
      if (lineWidthLimit > 0.0f) {
        float limitedWidth = lineWidthLimit * 8.0f;  // Approximate char width
        if (limitedWidth < textArea.width) {
          float offset = (textArea.width - limitedWidth) / 2.0f;
          return {textArea.x + offset, textArea.y, limitedWidth, textArea.height};
        }
      }
      return textArea;
    }
    
    // Paged mode: return area within page margins
    float marginScaled = pageMargin * pageScale;
    return {
      pageOffsetX + marginScaled,
      textArea.y + 10.0f + marginScaled,  // 10px for page shadow
      pageDisplayWidth - 2.0f * marginScaled,
      pageDisplayHeight - 2.0f * marginScaled
    };
  }
  
  // Toggle between paged and pageless modes
  void togglePageMode() {
    pageMode = (pageMode == PageMode::Pageless) ? PageMode::Paged : PageMode::Pageless;
    updateLayout(screenWidth, screenHeight);
  }
  
  // Set line width limit (0 to disable)
  void setLineWidthLimit(float chars) {
    lineWidthLimit = chars;
  }
};

// Component for test mode configuration
struct TestConfigComponent : public afterhours::BaseComponent {
  bool enabled = false;
  std::string screenshotDir = "output/screenshots";
  int frameLimit = 0;
  int frameCount = 0;
};

} // namespace ecs
