#include "preload.h"

#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/files.h>
#include <afterhours/src/plugins/ui/theme.h>

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>

#include "fonts/font_loader.h"
#include "input_mapping.h"
#include "rl.h"
#include "settings.h"
#include "ui/theme.h"
#include "ui/ui_context.h"
#include "util/logging.h"

using namespace afterhours;

#ifdef AFTER_HOURS_ENABLE_MCP
extern bool g_mcp_mode;

// Custom log callback that writes to stderr instead of stdout
static void mcp_trace_log_callback(int logLevel, const char *text,
                                   va_list args) {
    if (logLevel < raylib::LOG_ERROR) {
        return;  // In MCP mode, only log errors
    }
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    fprintf(stderr, "%s\n", buffer);
}
#endif

static void load_gamepad_mappings() {
    std::ifstream ifs(
        files::get_resource_path("", "gamecontrollerdb.txt").string().c_str());
    if (!ifs.is_open()) {
        log_warn("failed to load game controller db");
        return;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    input::set_gamepad_mappings(buffer.str().c_str());
}

Preload::Preload() {}

Preload &Preload::init(const char *title) {
    {
        SCOPED_TIMER("files::init");
        files::init("Prime Pressure", "resources");
    }

    int width = Settings::get().get_screen_width();
    int height = Settings::get().get_screen_height();

    // In MCP mode, redirect raylib logs to stderr to keep stdout clean for JSON
#ifdef AFTER_HOURS_ENABLE_MCP
    if (g_mcp_mode) {
        raylib::SetTraceLogCallback(mcp_trace_log_callback);
    }
#endif

    // Set log level BEFORE InitWindow to suppress init messages
    raylib::SetTraceLogLevel(raylib::LOG_ERROR);

    // Set config flags BEFORE InitWindow for faster setup
    // FLAG_WINDOW_RESIZABLE set here avoids SetWindowState call after
    raylib::SetConfigFlags(raylib::FLAG_WINDOW_RESIZABLE);

    {
        SCOPED_TIMER("InitWindow");
        raylib::InitWindow(width, height, title);
        // Note: SetWindowSize removed - redundant, size already in InitWindow
    }

    raylib::SetTargetFPS(200);

    // Skip audio initialization for word processor - not needed
    // Audio can be lazy-initialized later if sound effects are added
    // This saves ~150-900ms on startup
    // {
    //     SCOPED_TIMER("InitAudioDevice");
    //     raylib::SetAudioStreamBufferSizeDefault(4096);
    //     raylib::InitAudioDevice();
    //     if (!raylib::IsAudioDeviceReady()) {
    //         log_warn("audio device not ready; continuing without audio");
    //     }
    //     raylib::SetMasterVolume(1.f);
    // }

    raylib::SetExitKey(0);

    // Skip gamepad mappings - word processor doesn't need gamepad support
    // load_gamepad_mappings();

    return *this;
}

// CJK codepoint generation moved to fonts::FontLoader for lazy loading
// See fonts/font_loader.h for loadCJKFontsIfNeeded()

Preload &Preload::make_singleton() {
    auto &sophie = EntityHelper::createEntity();
    {
        {
            SCOPED_TIMER("Afterhours singleton setup");
            input::add_singleton_components(sophie, get_mapping());
            window_manager::add_singleton_components(sophie, 200);
            ui::add_singleton_components<ui_imm::InputAction>(sophie);
        }

        // Load only ONE essential font at startup for fastest launch
        std::string english_font =
            files::get_resource_path("fonts", "EBGaramond-Regular.ttf").string();

        {
            SCOPED_TIMER("Load default font");
            auto& fontMgr = sophie.get<ui::FontManager>();
            // Single font load - used as default for everything initially
            fontMgr.load_font(ui::UIComponent::DEFAULT_FONT, english_font.c_str());
            // Alias the same font for other uses to avoid extra loads
            fontMgr.load_font(ui::UIComponent::SYMBOL_FONT, english_font.c_str());
            fontMgr.load_font("Garamond", english_font.c_str());
            
            // Load UI font for Win95 widgets and other UI rendering
            theme::UI_FONT = raylib::LoadFont(english_font.c_str());
            raylib::SetTextureFilter(theme::UI_FONT.texture, raylib::TEXTURE_FILTER_BILINEAR);
            theme::UI_FONT_LOADED = true;
        }

        // Register loaded fonts with FontLoader for P2 font listing
        fonts::FontLoader::get().loadStartupFonts(
            sophie.get<ui::FontManager>());

        {
            SCOPED_TIMER("Theme setup");
            // Use Win95 theme for consistent styling between legacy and Afterhours widgets
            ui_imm::initWin95Theme();

            ui::imm::UIStylingDefaults::get().set_grid_snapping(true);
        }

        sophie.addComponent<ui::AutoLayoutRoot>();
        sophie.addComponent<ui::UIComponentDebug>("sophie");
        sophie.addComponent<ui::UIComponent>(sophie.id)
            .set_desired_width(afterhours::ui::screen_pct(1.f))
            .set_desired_height(afterhours::ui::screen_pct(1.f))
            .enable_font(afterhours::ui::UIComponent::DEFAULT_FONT, 75.f);
    }
    return *this;
}

Preload::~Preload() {
    // Audio device cleanup skipped - not initialized for word processor
    // if (raylib::IsAudioDeviceReady()) {
    //     raylib::CloseAudioDevice();
    // }
    if (raylib::IsWindowReady()) {
        raylib::CloseWindow();
    }
}
