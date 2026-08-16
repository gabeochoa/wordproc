#pragma once

// Must precede any afterhours header: it selects the graphics backend, and the
// backend decides the layout of TextureType et al. A translation unit that
// reaches afterhours first gets the "none" backend's types and then fails to
// compile against the sokol drawing helpers.
#include "external.h"

#include <afterhours/src/library.h>
#include <afterhours/src/plugins/window_manager.h>
#include <afterhours/src/singleton.h>

#include <memory>
#include <string>
#include <vector>

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

    // Recent files
    const std::vector<std::string>& get_recent_files() const;
    void add_recent_file(const std::string& path);
    void clear_recent_files();

    // UI scaling
    float get_ui_scale() const;
    void set_ui_scale(float scale);
};
