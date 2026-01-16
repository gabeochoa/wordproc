#include "editor/document_io.h"
#include "editor/text_buffer.h"
#include "editor/text_layout.h"
#include "input/action_map.h"
#include "preload.h"
#include "rl.h"
#include "settings.h"
#include "ui/menu_setup.h"
#include "ui/theme.h"
#include "ui/win95_widgets.h"
#include "util/drawing.h"
#include "util/logging.h"

#include <argh.h>
#include <chrono>
#include <format>
#include <filesystem>
#include <string>

#ifdef AFTER_HOURS_ENABLE_MCP
bool g_mcp_mode = false;
int g_saved_stdout_fd = -1;
#endif

// Status message for error/success reporting
struct StatusMessage {
  std::string text;
  double expiresAt = 0.0;  // Time when message expires
  bool isError = false;
};

// Test mode configuration
struct TestConfig {
  bool enabled = false;
  std::string screenshot_dir = "output/screenshots";
  int frame_limit = 0; // 0 = run forever
};

// Render text buffer with caret and selection (uses SoA layout for efficiency)
// Uses per-glyph metrics via MeasureText for accurate caret/selection positioning
void renderTextBuffer(const TextBuffer &buffer, raylib::Rectangle textArea,
                      bool caretVisible, int fontSize, int lineHeight,
                      int scrollOffset = 0) {
  std::size_t lineCount = buffer.lineCount();
  CaretPosition caret = buffer.caret();
  bool hasSelection = buffer.hasSelection();
  CaretPosition selStart = buffer.selectionStart();
  CaretPosition selEnd = buffer.selectionEnd();

  int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING;
  
  // Skip lines above the scroll offset
  std::size_t startRow = static_cast<std::size_t>(scrollOffset);
  if (startRow >= lineCount) startRow = lineCount > 0 ? lineCount - 1 : 0;
  
  // Adjust starting y position for visible lines
  for (std::size_t row = startRow; row < lineCount; ++row) {
    LineSpan span = buffer.lineSpan(row);
    int x = static_cast<int>(textArea.x) + theme::layout::TEXT_PADDING;
    
    // Get the full line text (needed for measurements)
    std::string line = (span.length > 0) ? buffer.lineString(row) : "";

    // Draw selection highlight for this line using per-glyph measurements
    if (hasSelection) {
      bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
      if (lineInSelection) {
        std::size_t startCol = (row == selStart.row) ? selStart.column : 0;
        std::size_t endCol = (row == selEnd.row) ? selEnd.column : span.length;
        
        if (startCol < endCol && !line.empty()) {
          // Measure text width up to startCol and endCol using per-glyph metrics
          std::string beforeSel = line.substr(0, startCol);
          std::string selectedText = line.substr(startCol, endCol - startCol);
          
          int selX = x + raylib::MeasureText(beforeSel.c_str(), fontSize);
          int selWidth = raylib::MeasureText(selectedText.c_str(), fontSize);
          raylib::DrawRectangle(selX, y, selWidth, lineHeight, theme::SELECTION_BG);
        }
      }
    }

    // Draw text
    if (!line.empty()) {
      raylib::DrawText(line.c_str(), x, y, fontSize, theme::TEXT_COLOR);
    }

    // Draw caret on this line using per-glyph measurements
    if (caretVisible && row == caret.row) {
      // Measure text width from start to caret position for accurate positioning
      std::string beforeCaret = line.substr(0, std::min(caret.column, line.length()));
      int caretX = x + raylib::MeasureText(beforeCaret.c_str(), fontSize);
      raylib::DrawRectangle(caretX, y, 2, lineHeight, theme::CARET_COLOR);
    }

    y += lineHeight;
    
    // Stop if we're past the visible area
    if (y > static_cast<int>(textArea.y + textArea.height)) {
      break;
    }
  }
}

// Take a screenshot with a descriptive name
void takeScreenshot(const std::string &dir, const std::string &name) {
  std::filesystem::create_directories(dir);
  std::string path = dir + "/" + name + ".png";
  raylib::TakeScreenshot(path.c_str());
}

int main(int argc, char *argv[]) {
  argh::parser cmdl(argc, argv);
  
  TestConfig testConfig;
  testConfig.enabled = cmdl["--test-mode"];
  cmdl("--screenshot-dir", "output/screenshots") >> testConfig.screenshot_dir;
  cmdl("--frame-limit", 0) >> testConfig.frame_limit;
  
  // Headless benchmark mode - measures file load time without opening window
  bool benchmarkMode = cmdl["--benchmark"];
  
  std::string loadFile;
  cmdl(1, "") >> loadFile; // First positional argument is file to open
  
  // Track startup time
  auto startTime = std::chrono::high_resolution_clock::now();

  // Headless benchmark: just load file and report timing
  if (benchmarkMode) {
    TextBuffer buffer;
    
    auto loadStart = std::chrono::high_resolution_clock::now();
    
    if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
      loadTextFile(buffer, loadFile);
    }
    
    auto loadEnd = std::chrono::high_resolution_clock::now();
    auto loadMs = std::chrono::duration_cast<std::chrono::microseconds>(
        loadEnd - loadStart).count() / 1000.0;
    auto totalMs = std::chrono::duration_cast<std::chrono::microseconds>(
        loadEnd - startTime).count() / 1000.0;
    
    // Get file size
    std::size_t fileSize = 0;
    if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
      fileSize = std::filesystem::file_size(loadFile);
    }
    
    // Output CSV-friendly format using logging
    LOG_INFO("file=%s,size=%zu,lines=%zu,chars=%zu,load_ms=%.3f,total_ms=%.3f,target=100,pass=%s",
             loadFile.c_str(),
             fileSize,
             buffer.lineCount(),
             buffer.getText().size(),
             loadMs,
             totalMs,
             totalMs <= 100.0 ? "true" : "false");
    
    return totalMs <= 100.0 ? 0 : 1;
  }

  Settings::get().load_save_file(800, 600);

  Preload::get().init("Wordproc - Untitled").make_singleton();
  Settings::get().refresh_settings();

  TextBuffer buffer;
  const std::string doc_path = "output/document.wpdoc";
  std::string currentFilePath = loadFile;
  bool isDirty = false;
  StatusMessage statusMsg;
  
  // Helper to set status message
  auto setStatus = [&statusMsg](const std::string& msg, bool isError = false) {
    statusMsg.text = msg;
    statusMsg.isError = isError;
    statusMsg.expiresAt = raylib::GetTime() + STATUS_MESSAGE_DURATION;
  };
  
  // Load file if specified
  if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
    auto result = loadTextFileEx(buffer, loadFile);
    if (result.success) {
      if (result.usedFallback && !result.error.empty()) {
        setStatus(result.error, false);  // Show fallback warning
      }
    } else {
      setStatus("Load failed: " + result.error, true);
    }
  }

  // Setup Win95-style menus using centralized menu setup
  std::vector<win95::Menu> menus = menu_setup::createMenuBar();
  
  // Dialog state
  bool showAboutDialog = false;

  // Measure startup time
  auto readyTime = std::chrono::high_resolution_clock::now();
  auto startupMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      readyTime - startTime).count();
  
  if (testConfig.enabled) {
    LOG_INFO("Startup time: %lld ms", static_cast<long long>(startupMs));
    if (startupMs > 100) {
      LOG_WARNING("Startup time exceeds 100ms target!");
    }
  }

  int frameCount = 0;
  double caretBlinkTimer = 0.0;
  bool caretVisible = true;
  int scrollOffset = 0;  // Scroll offset in lines

  while (!raylib::WindowShouldClose()) {
    float dt = raylib::GetFrameTime();
    frameCount++;
    
    // Caret blinking
    caretBlinkTimer += dt;
    if (caretBlinkTimer >= 0.5) {
      caretBlinkTimer = 0.0;
      caretVisible = !caretVisible;
    }

    // Handle text input
    int codepoint = raylib::GetCharPressed();
    while (codepoint > 0) {
      if (codepoint >= 32) {
        buffer.insertChar(static_cast<char>(codepoint));
        isDirty = true;
        caretVisible = true;
        caretBlinkTimer = 0.0;
      }
      codepoint = raylib::GetCharPressed();
    }

    if (raylib::IsKeyPressed(raylib::KEY_ENTER) ||
        raylib::IsKeyPressed(raylib::KEY_KP_ENTER)) {
      buffer.insertChar('\n');
      isDirty = true;
    }
    if (raylib::IsKeyPressed(raylib::KEY_BACKSPACE)) {
      buffer.backspace();
      isDirty = true;
    }
    if (raylib::IsKeyPressed(raylib::KEY_DELETE)) {
      buffer.del();
      isDirty = true;
    }

    bool ctrl_down = raylib::IsKeyDown(raylib::KEY_LEFT_CONTROL) ||
                     raylib::IsKeyDown(raylib::KEY_RIGHT_CONTROL);
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_S)) {
      std::string savePath = currentFilePath.empty() ? doc_path : currentFilePath;
      auto result = saveTextFileEx(buffer, savePath);
      if (result.success) {
        isDirty = false;
        currentFilePath = savePath;
        setStatus("Saved: " + std::filesystem::path(savePath).filename().string());
      } else {
        setStatus("Save failed: " + result.error, true);
      }
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_O)) {
      auto result = loadTextFileEx(buffer, doc_path);
      if (result.success) {
        currentFilePath = doc_path;
        isDirty = false;
        if (result.usedFallback) {
          setStatus("Opened (plain text): " + std::filesystem::path(doc_path).filename().string(), false);
        } else {
          setStatus("Opened: " + std::filesystem::path(doc_path).filename().string());
        }
      } else {
        setStatus("Open failed: " + result.error, true);
      }
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_B)) {
      TextStyle style = buffer.textStyle();
      style.bold = !style.bold;
      buffer.setTextStyle(style);
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_I)) {
      TextStyle style = buffer.textStyle();
      style.italic = !style.italic;
      buffer.setTextStyle(style);
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_ONE)) {
      TextStyle style = buffer.textStyle();
      style.font = "Gaegu-Bold";
      buffer.setTextStyle(style);
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_TWO)) {
      TextStyle style = buffer.textStyle();
      style.font = "EBGaramond-Regular";
      buffer.setTextStyle(style);
    }
    // Font size: Ctrl+Plus/Equal to increase, Ctrl+Minus to decrease
    if (ctrl_down && (raylib::IsKeyPressed(raylib::KEY_EQUAL) ||
                      raylib::IsKeyPressed(raylib::KEY_KP_ADD))) {
      TextStyle style = buffer.textStyle();
      style.fontSize = std::min(72, style.fontSize + 2);
      buffer.setTextStyle(style);
    }
    if (ctrl_down && (raylib::IsKeyPressed(raylib::KEY_MINUS) ||
                      raylib::IsKeyPressed(raylib::KEY_KP_SUBTRACT))) {
      TextStyle style = buffer.textStyle();
      style.fontSize = std::max(8, style.fontSize - 2);
      buffer.setTextStyle(style);
    }
    // Reset font size: Ctrl+0
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_ZERO)) {
      TextStyle style = buffer.textStyle();
      style.fontSize = 16;
      buffer.setTextStyle(style);
    }
    
    // Clipboard operations
    // Ctrl+C - Copy
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_C)) {
      if (buffer.hasSelection()) {
        std::string selected = buffer.getSelectedText();
        if (!selected.empty()) {
          raylib::SetClipboardText(selected.c_str());
        }
      }
    }
    
    // Ctrl+X - Cut
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_X)) {
      if (buffer.hasSelection()) {
        std::string selected = buffer.getSelectedText();
        if (!selected.empty()) {
          raylib::SetClipboardText(selected.c_str());
          buffer.deleteSelection();
          isDirty = true;
        }
      }
    }
    
    // Ctrl+V - Paste
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_V)) {
      const char* clipText = raylib::GetClipboardText();
      if (clipText && clipText[0] != '\0') {
        buffer.insertText(clipText);
        isDirty = true;
      }
    }
    
    // Ctrl+A - Select All
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_A)) {
      buffer.selectAll();
    }
    
    // Ctrl+Z - Undo
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_Z)) {
      if (buffer.canUndo()) {
        buffer.undo();
        isDirty = true;
        caretVisible = true;
        caretBlinkTimer = 0.0;
      }
    }
    
    // Ctrl+Y - Redo
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_Y)) {
      if (buffer.canRedo()) {
        buffer.redo();
        isDirty = true;
        caretVisible = true;
        caretBlinkTimer = 0.0;
      }
    }

    bool shift_down = raylib::IsKeyDown(raylib::KEY_LEFT_SHIFT) ||
                      raylib::IsKeyDown(raylib::KEY_RIGHT_SHIFT);

    // Helper lambda for navigation with selection support
    auto navigateWithSelection = [&](auto moveFunc) {
      CaretPosition before = buffer.caret();
      if (shift_down && !buffer.hasSelection()) {
        buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        buffer.clearSelection();
      }
      moveFunc();
      if (shift_down) {
        buffer.updateSelectionToCaret();
      }
      caretVisible = true;
      caretBlinkTimer = 0.0;
    };

    if (raylib::IsKeyPressed(raylib::KEY_LEFT)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { buffer.moveWordLeft(); });
      } else {
        navigateWithSelection([&]() { buffer.moveLeft(); });
      }
    }

    if (raylib::IsKeyPressed(raylib::KEY_RIGHT)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { buffer.moveWordRight(); });
      } else {
        navigateWithSelection([&]() { buffer.moveRight(); });
      }
    }

    if (raylib::IsKeyPressed(raylib::KEY_UP)) {
      navigateWithSelection([&]() { buffer.moveUp(); });
    }

    if (raylib::IsKeyPressed(raylib::KEY_DOWN)) {
      navigateWithSelection([&]() { buffer.moveDown(); });
    }

    // Home/End navigation
    if (raylib::IsKeyPressed(raylib::KEY_HOME)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { buffer.moveToDocumentStart(); });
      } else {
        navigateWithSelection([&]() { buffer.moveToLineStart(); });
      }
    }

    if (raylib::IsKeyPressed(raylib::KEY_END)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { buffer.moveToDocumentEnd(); });
      } else {
        navigateWithSelection([&]() { buffer.moveToLineEnd(); });
      }
    }

    // Page Up/Down (assuming ~20 lines per page for now)
    constexpr std::size_t LINES_PER_PAGE = 20;
    if (raylib::IsKeyPressed(raylib::KEY_PAGE_UP)) {
      navigateWithSelection([&]() { buffer.movePageUp(LINES_PER_PAGE); });
    }

    if (raylib::IsKeyPressed(raylib::KEY_PAGE_DOWN)) {
      navigateWithSelection([&]() { buffer.movePageDown(LINES_PER_PAGE); });
    }

    // Mouse wheel / trackpad scrolling
    float wheelMove = raylib::GetMouseWheelMove();
    if (wheelMove != 0.0f) {
      // Scroll 3 lines per wheel notch
      int scrollLines = static_cast<int>(-wheelMove * 3);
      scrollOffset += scrollLines;
    }

    // Calculate window layout
    int screenWidth = raylib::GetScreenWidth();
    int screenHeight = raylib::GetScreenHeight();

    // Title bar area
    raylib::Rectangle titleBar = {0, 0, static_cast<float>(screenWidth),
                                   static_cast<float>(TITLE_BAR_HEIGHT)};
    
    // Menu bar area
    raylib::Rectangle menuBar = {0, static_cast<float>(TITLE_BAR_HEIGHT),
                                  static_cast<float>(screenWidth),
                                  static_cast<float>(MENU_BAR_HEIGHT)};
    
    // Status bar area
    raylib::Rectangle statusBar = {0, static_cast<float>(screenHeight - STATUS_BAR_HEIGHT),
                                    static_cast<float>(screenWidth),
                                    static_cast<float>(STATUS_BAR_HEIGHT)};
    
    // Text editing area
    float textAreaTop = static_cast<float>(TITLE_BAR_HEIGHT + MENU_BAR_HEIGHT + BORDER_WIDTH);
    float textAreaHeight = static_cast<float>(screenHeight - TITLE_BAR_HEIGHT - 
                                               MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT - 
                                               2 * BORDER_WIDTH);
    raylib::Rectangle textArea = {static_cast<float>(BORDER_WIDTH), textAreaTop,
                                   static_cast<float>(screenWidth - 2 * BORDER_WIDTH),
                                   textAreaHeight};
    
    // Calculate visible lines for auto-scroll
    TextStyle currentStyle = buffer.textStyle();
    int currentLineHeight = currentStyle.fontSize + 4;
    int visibleLines = static_cast<int>((textAreaHeight - 2 * TEXT_PADDING) / currentLineHeight);
    if (visibleLines < 1) visibleLines = 1;
    
    // Auto-scroll to keep caret visible
    CaretPosition caretPos = buffer.caret();
    int caretRow = static_cast<int>(caretPos.row);
    if (caretRow < scrollOffset) {
      scrollOffset = caretRow;
    } else if (caretRow >= scrollOffset + visibleLines) {
      scrollOffset = caretRow - visibleLines + 1;
    }
    
    // Clamp scroll offset
    int maxScroll = static_cast<int>(buffer.lineCount()) - visibleLines;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    // === DRAWING ===
    raylib::BeginDrawing();
    raylib::ClearBackground(colors::WINDOW_BG);

    // Draw title bar
    raylib::DrawRectangleRec(titleBar, colors::TITLE_BAR);
    std::string title = "Wordproc";
    if (!currentFilePath.empty()) {
      title += " - " + std::filesystem::path(currentFilePath).filename().string();
    } else {
      title += " - Untitled";
    }
    if (isDirty) {
      title += " *";
    }
    raylib::DrawText(title.c_str(), 4, 4, FONT_SIZE, colors::TITLE_TEXT);

    // Draw menu bar background
    raylib::DrawRectangleRec(menuBar, colors::WINDOW_BG);
    drawRaisedBorder(menuBar);
    
    // Draw interactive menus (must be done before other UI to get proper z-order)
    int menuResult = win95::DrawMenuBar(menus, TITLE_BAR_HEIGHT, MENU_BAR_HEIGHT);
    
    // Handle menu actions
    if (menuResult >= 0) {
      int menuIndex = menuResult / 100;
      int itemIndex = menuResult % 100;
      
      if (menuIndex == 0) { // File menu
        switch (itemIndex) {
          case 0: // New
            buffer.setText("");
            currentFilePath.clear();
            isDirty = false;
            break;
          case 1: // Open
            {
              auto result = loadTextFileEx(buffer, doc_path);
              if (result.success) {
                currentFilePath = doc_path;
                isDirty = false;
                if (result.usedFallback) {
                  setStatus("Opened (plain text): " + std::filesystem::path(doc_path).filename().string());
                } else {
                  setStatus("Opened: " + std::filesystem::path(doc_path).filename().string());
                }
              } else {
                setStatus("Open failed: " + result.error, true);
              }
            }
            break;
          case 2: // Save
            {
              std::string savePath = currentFilePath.empty() ? doc_path : currentFilePath;
              auto result = saveTextFileEx(buffer, savePath);
              if (result.success) {
                isDirty = false;
                currentFilePath = savePath;
                setStatus("Saved: " + std::filesystem::path(savePath).filename().string());
              } else {
                setStatus("Save failed: " + result.error, true);
              }
            }
            break;
          case 5: // Exit
            break; // WindowShouldClose will handle
          default:
            break;
        }
      } else if (menuIndex == 1) { // Edit menu
        switch (itemIndex) {
          case 0: // Undo
            if (buffer.canUndo()) {
              buffer.undo();
              isDirty = true;
            }
            break;
          case 1: // Redo
            if (buffer.canRedo()) {
              buffer.redo();
              isDirty = true;
            }
            break;
          case 3: // Cut
            if (buffer.hasSelection()) {
              std::string selected = buffer.getSelectedText();
              if (!selected.empty()) {
                raylib::SetClipboardText(selected.c_str());
                buffer.deleteSelection();
                isDirty = true;
              }
            }
            break;
          case 4: // Copy
            if (buffer.hasSelection()) {
              std::string selected = buffer.getSelectedText();
              if (!selected.empty()) {
                raylib::SetClipboardText(selected.c_str());
              }
            }
            break;
          case 5: // Paste
            {
              const char* clipText = raylib::GetClipboardText();
              if (clipText && clipText[0] != '\0') {
                buffer.insertText(clipText);
                isDirty = true;
              }
            }
            break;
          case 7: // Select All
            buffer.selectAll();
            break;
          default:
            break;
        }
      } else if (menuIndex == 2) { // Format menu
        TextStyle style = buffer.textStyle();
        switch (itemIndex) {
          case 0: // Bold
            style.bold = !style.bold;
            buffer.setTextStyle(style);
            break;
          case 1: // Italic
            style.italic = !style.italic;
            buffer.setTextStyle(style);
            break;
          case 3: // Font: Gaegu
            style.font = "Gaegu-Bold";
            buffer.setTextStyle(style);
            break;
          case 4: // Font: Garamond
            style.font = "EBGaramond-Regular";
            buffer.setTextStyle(style);
            break;
          case 6: // Increase Size
            style.fontSize = std::min(72, style.fontSize + 2);
            buffer.setTextStyle(style);
            break;
          case 7: // Decrease Size
            style.fontSize = std::max(8, style.fontSize - 2);
            buffer.setTextStyle(style);
            break;
          case 8: // Reset Size
            style.fontSize = 16;
            buffer.setTextStyle(style);
            break;
          default:
            break;
        }
      } else if (menuIndex == 3) { // Help menu
        if (itemIndex == 0) { // About
          showAboutDialog = true;
        }
      }
    }

    // Draw text area background
    raylib::DrawRectangleRec(textArea, colors::TEXT_AREA_BG);
    drawSunkenBorder(textArea);

    // Render text buffer with dynamic font size
    TextStyle style = buffer.textStyle();
    int fontSize = style.fontSize;
    int lineHeight = fontSize + 4;  // Add some line spacing
    renderTextBuffer(buffer, textArea, caretVisible, fontSize, lineHeight, scrollOffset);

    // Draw status bar
    raylib::DrawRectangleRec(statusBar, colors::STATUS_BAR);
    drawRaisedBorder(statusBar);
    
    // Check if there's an active status message
    double currentTime = raylib::GetTime();
    if (!statusMsg.text.empty() && currentTime < statusMsg.expiresAt) {
      // Display status message
      raylib::Color msgColor = statusMsg.isError ? 
        raylib::Color{200, 0, 0, 255} : raylib::Color{0, 100, 0, 255};
      raylib::DrawText(statusMsg.text.c_str(), 4, screenHeight - STATUS_BAR_HEIGHT + 2,
                       FONT_SIZE - 2, msgColor);
    } else {
      // Display normal status: line/col, formatting, etc.
      CaretPosition caret = buffer.caret();
      char statusText[128];
      std::snprintf(statusText, sizeof(statusText), 
                    "Ln %zu, Col %zu | %s%s| %dpt | %s",
                    caret.row + 1, caret.column + 1,
                    style.bold ? "B " : "",
                    style.italic ? "I " : "",
                    style.fontSize,
                    style.font.c_str());
      raylib::DrawText(statusText, 4, screenHeight - STATUS_BAR_HEIGHT + 2,
                       FONT_SIZE - 2, colors::TEXT_COLOR);
    }

    // Draw About dialog if active
    if (showAboutDialog) {
      raylib::Rectangle dialogRect = {
        static_cast<float>(screenWidth / 2 - 150),
        static_cast<float>(screenHeight / 2 - 75),
        300, 150
      };
      int result = win95::DrawMessageDialog(dialogRect, "About Wordproc",
        "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt with Afterhours.", false);
      if (result >= 0) {
        showAboutDialog = false;
      }
    }

    raylib::EndDrawing();

    // Test mode: take screenshots and exit
    if (testConfig.enabled) {
      if (frameCount == 1) {
        takeScreenshot(testConfig.screenshot_dir, "01_startup");
      }
      if (testConfig.frame_limit > 0 && frameCount >= testConfig.frame_limit) {
        takeScreenshot(testConfig.screenshot_dir, "final");
        break;
      }
    }
  }

  Settings::get().write_save_file();
  return 0;
}
