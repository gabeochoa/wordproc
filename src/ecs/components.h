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

// Component for window layout calculations
struct LayoutComponent : public afterhours::BaseComponent {
  float titleBarHeight = 20.0f;
  float menuBarHeight = 20.0f;
  float statusBarHeight = 18.0f;
  float borderWidth = 2.0f;
  float textPadding = 4.0f;
  
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
