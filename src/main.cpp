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

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    raylib::ClearBackground(raylib::RAYWHITE);
    raylib::EndDrawing();
  }

  Settings::get().write_save_file();
  return 0;
}
