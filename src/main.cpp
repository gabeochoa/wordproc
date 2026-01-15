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

    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::RAYWHITE);
    raylib::EndDrawing();
  }

  Settings::get().write_save_file();
  return 0;
}
