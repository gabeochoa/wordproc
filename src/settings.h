#pragma once

#include <memory>

#include <afterhours/src/library.h>
#include <afterhours/src/plugins/window_manager.h>
#include <afterhours/src/singleton.h>

struct S_Data;

SINGLETON_FWD(Settings)
struct Settings {
  SINGLETON(Settings)

  S_Data *data;

  Settings();
  ~Settings();

  Settings(const Settings &) = delete;
  void operator=(const Settings &) = delete;

  bool load_save_file(int, int);
  void write_save_file();

  void reset();
  void refresh_settings();

  int get_screen_width() const;
  int get_screen_height() const;
  void update_resolution(afterhours::window_manager::Resolution);

  bool &get_fullscreen_enabled();
  void toggle_fullscreen();
  
  // Auto-save support: when enabled, settings are written after each change
  bool auto_save_enabled = true;
  void save_if_auto();  // Call write_save_file() if auto_save_enabled
};
