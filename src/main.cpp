#include "editor/text_buffer.h"
#include "preload.h"
#include "rl.h"
#include "settings.h"

#ifdef AFTER_HOURS_ENABLE_MCP
bool g_mcp_mode = false;
int g_saved_stdout_fd = -1;
#endif

int main() {
  Settings::get().load_save_file(1280, 720);

  Preload::get().init("Wordproc").make_singleton();
  Settings::get().refresh_settings();

  TextBuffer buffer;

  while (!raylib::WindowShouldClose()) {
    int codepoint = raylib::GetCharPressed();
    while (codepoint > 0) {
      if (codepoint >= 32) {
        buffer.insertChar(static_cast<char>(codepoint));
      }
      codepoint = raylib::GetCharPressed();
    }

    if (raylib::IsKeyPressed(raylib::KEY_ENTER) ||
        raylib::IsKeyPressed(raylib::KEY_KP_ENTER)) {
      buffer.insertChar('\n');
    }
    if (raylib::IsKeyPressed(raylib::KEY_BACKSPACE)) {
      buffer.backspace();
    }
    if (raylib::IsKeyPressed(raylib::KEY_DELETE)) {
      buffer.del();
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
    }

    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::RAYWHITE);
    raylib::EndDrawing();
  }

  Settings::get().write_save_file();
  return 0;
}
