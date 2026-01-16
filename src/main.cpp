#include "editor/document_io.h"
#include "editor/text_buffer.h"
#include "editor/text_layout.h"
#include "preload.h"
#include "rl.h"
#include "settings.h"

#include <argh.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#ifdef AFTER_HOURS_ENABLE_MCP
bool g_mcp_mode = false;
int g_saved_stdout_fd = -1;
#endif

// Win95-style colors
namespace colors {
constexpr raylib::Color WINDOW_BG = {192, 192, 192, 255};     // Win95 gray
constexpr raylib::Color TITLE_BAR = {0, 0, 128, 255};         // Win95 blue
constexpr raylib::Color TITLE_TEXT = {255, 255, 255, 255};    // White
constexpr raylib::Color TEXT_AREA_BG = {255, 255, 255, 255};  // White
constexpr raylib::Color TEXT_COLOR = {0, 0, 0, 255};          // Black
constexpr raylib::Color CARET_COLOR = {0, 0, 0, 255};         // Black
constexpr raylib::Color SELECTION_BG = {0, 0, 128, 255};      // Blue highlight
// constexpr raylib::Color SELECTION_TEXT = {255, 255, 255, 255};// White text on selection (TODO: use for styled selection text)
constexpr raylib::Color BORDER_LIGHT = {255, 255, 255, 255};  // 3D border light
constexpr raylib::Color BORDER_DARK = {128, 128, 128, 255};   // 3D border dark
constexpr raylib::Color STATUS_BAR = {192, 192, 192, 255};    // Status bar gray
} // namespace colors

// Configuration
constexpr int FONT_SIZE = 16;  // UI font size (title, menus, status bar)
constexpr int TITLE_BAR_HEIGHT = 24;
constexpr int MENU_BAR_HEIGHT = 20;
constexpr int STATUS_BAR_HEIGHT = 20;
constexpr int TEXT_PADDING = 8;
constexpr int BORDER_WIDTH = 3;

// Test mode configuration
struct TestConfig {
  bool enabled = false;
  std::string screenshot_dir = "output/screenshots";
  int frame_limit = 0; // 0 = run forever
};

// Draw Win95-style 3D border (raised effect)
void drawRaisedBorder(raylib::Rectangle rect) {
  // Top and left (light)
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
                   colors::BORDER_LIGHT);
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                   static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_LIGHT);
  // Bottom and right (dark)
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_DARK);
  raylib::DrawLine(static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_DARK);
}

// Draw Win95-style 3D border (sunken effect for text area)
void drawSunkenBorder(raylib::Rectangle rect) {
  // Top and left (dark)
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
                   colors::BORDER_DARK);
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                   static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_DARK);
  // Bottom and right (light)
  raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_LIGHT);
  raylib::DrawLine(static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
                   static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y + rect.height),
                   colors::BORDER_LIGHT);
}

// Render text buffer with caret and selection
void renderTextBuffer(const TextBuffer &buffer, raylib::Rectangle textArea,
                      bool caretVisible, int fontSize, int lineHeight) {
  const auto &lines = buffer.lines();
  CaretPosition caret = buffer.caret();
  bool hasSelection = buffer.hasSelection();
  CaretPosition selStart = buffer.selectionStart();
  CaretPosition selEnd = buffer.selectionEnd();

  // Approximate character width (monospace assumption, will be refined later)
  int charWidth = fontSize / 2;
  if (charWidth < 4) charWidth = 4;

  int y = static_cast<int>(textArea.y) + TEXT_PADDING;
  
  for (std::size_t row = 0; row < lines.size(); ++row) {
    const std::string &line = lines[row];
    int x = static_cast<int>(textArea.x) + TEXT_PADDING;

    // Draw selection highlight for this line
    if (hasSelection) {
      bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
      if (lineInSelection) {
        std::size_t startCol = (row == selStart.row) ? selStart.column : 0;
        std::size_t endCol = (row == selEnd.row) ? selEnd.column : line.size();
        
        if (startCol < endCol) {
          int selX = x + static_cast<int>(startCol) * charWidth;
          int selWidth = static_cast<int>(endCol - startCol) * charWidth;
          raylib::DrawRectangle(selX, y, selWidth, lineHeight, colors::SELECTION_BG);
        }
      }
    }

    // Draw text
    if (!line.empty()) {
      raylib::DrawText(line.c_str(), x, y, fontSize, colors::TEXT_COLOR);
    }

    // Draw caret on this line
    if (caretVisible && row == caret.row) {
      int caretX = x + static_cast<int>(caret.column) * charWidth;
      raylib::DrawRectangle(caretX, y, 2, lineHeight, colors::CARET_COLOR);
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
  
  std::string loadFile;
  cmdl(1, "") >> loadFile; // First positional argument is file to open
  
  // Track startup time
  auto startTime = std::chrono::high_resolution_clock::now();

  Settings::get().load_save_file(800, 600);

  Preload::get().init("Wordproc - Untitled").make_singleton();
  Settings::get().refresh_settings();

  TextBuffer buffer;
  const std::string doc_path = "output/document.wpdoc";
  std::string currentFilePath = loadFile;
  bool isDirty = false;
  
  // Load file if specified
  if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
    loadTextFile(buffer, loadFile);
  }

  // Measure startup time
  auto readyTime = std::chrono::high_resolution_clock::now();
  auto startupMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      readyTime - startTime).count();
  
  if (testConfig.enabled) {
    std::printf("Startup time: %lld ms\n", static_cast<long long>(startupMs));
    if (startupMs > 100) {
      std::printf("WARNING: Startup time exceeds 100ms target!\n");
    }
  }

  int frameCount = 0;
  double caretBlinkTimer = 0.0;
  bool caretVisible = true;

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
      if (saveTextFile(buffer, savePath)) {
        isDirty = false;
        currentFilePath = savePath;
      }
    }
    if (ctrl_down && raylib::IsKeyPressed(raylib::KEY_O)) {
      if (loadTextFile(buffer, doc_path)) {
        currentFilePath = doc_path;
        isDirty = false;
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

    bool shift_down = raylib::IsKeyDown(raylib::KEY_LEFT_SHIFT) ||
                      raylib::IsKeyDown(raylib::KEY_RIGHT_SHIFT);

    if (raylib::IsKeyPressed(raylib::KEY_LEFT)) {
      CaretPosition before = buffer.caret();
      if (shift_down && !buffer.hasSelection()) {
        buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        buffer.clearSelection();
      }
      buffer.moveLeft();
      if (shift_down) {
        buffer.updateSelectionToCaret();
      }
      caretVisible = true;
      caretBlinkTimer = 0.0;
    }

    if (raylib::IsKeyPressed(raylib::KEY_RIGHT)) {
      CaretPosition before = buffer.caret();
      if (shift_down && !buffer.hasSelection()) {
        buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        buffer.clearSelection();
      }
      buffer.moveRight();
      if (shift_down) {
        buffer.updateSelectionToCaret();
      }
      caretVisible = true;
      caretBlinkTimer = 0.0;
    }

    if (raylib::IsKeyPressed(raylib::KEY_UP)) {
      CaretPosition before = buffer.caret();
      if (shift_down && !buffer.hasSelection()) {
        buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        buffer.clearSelection();
      }
      buffer.moveUp();
      if (shift_down) {
        buffer.updateSelectionToCaret();
      }
      caretVisible = true;
      caretBlinkTimer = 0.0;
    }

    if (raylib::IsKeyPressed(raylib::KEY_DOWN)) {
      CaretPosition before = buffer.caret();
      if (shift_down && !buffer.hasSelection()) {
        buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        buffer.clearSelection();
      }
      buffer.moveDown();
      if (shift_down) {
        buffer.updateSelectionToCaret();
      }
      caretVisible = true;
      caretBlinkTimer = 0.0;
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

    // Draw menu bar
    raylib::DrawRectangleRec(menuBar, colors::WINDOW_BG);
    raylib::DrawText("File  Edit  Format  Help", 4,
                     TITLE_BAR_HEIGHT + 2, FONT_SIZE - 2, colors::TEXT_COLOR);
    drawRaisedBorder(menuBar);

    // Draw text area background
    raylib::DrawRectangleRec(textArea, colors::TEXT_AREA_BG);
    drawSunkenBorder(textArea);

    // Render text buffer with dynamic font size
    TextStyle style = buffer.textStyle();
    int fontSize = style.fontSize;
    int lineHeight = fontSize + 4;  // Add some line spacing
    renderTextBuffer(buffer, textArea, caretVisible, fontSize, lineHeight);

    // Draw status bar
    raylib::DrawRectangleRec(statusBar, colors::STATUS_BAR);
    drawRaisedBorder(statusBar);
    
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
