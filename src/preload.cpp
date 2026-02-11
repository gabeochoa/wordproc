#include "preload.h"

#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/files.h>
#include <afterhours/src/plugins/ui/theme.h>

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

// MCP log callback — Metal uses afterhours logging directly

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

Preload &Preload::init(const char * /*title*/) {
    {
        SCOPED_TIMER("files::init");
        files::init("Prime Pressure", "resources");
    }

    // Window creation, config flags, and target FPS are now handled by
    // afterhours::graphics::run() via RunConfig.  Nothing else to do here.

    afterhours::graphics::set_exit_key(0);

    // Skip audio initialization for word processor - not needed
    // Audio can be lazy-initialized later if sound effects are added
    // Audio initialization skipped — not yet ported to Metal backend

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

        // Load fonts for UI and document rendering
        std::string document_font =
            files::get_resource_path("fonts", "EBGaramond-Regular.ttf").string();
        std::string ui_font_path =
            files::get_resource_path("fonts", "Roboto-Regular.ttf").string();

        {
            SCOPED_TIMER("Load fonts");
            auto& fontMgr = sophie.get<ui::FontManager>();
            // Sans-serif font for all UI chrome (menus, toolbar, title bar, status bar)
            fontMgr.load_font(ui::UIComponent::DEFAULT_FONT, ui_font_path.c_str());
            fontMgr.load_font(ui::UIComponent::SYMBOL_FONT, ui_font_path.c_str());
            // Serif font available for document-related rendering
            fontMgr.load_font("Garamond", document_font.c_str());
            
            // Load sans-serif UI font for manual Win95 widget rendering (DrawUIText)
            theme::UI_FONT = afterhours::load_font_from_file(ui_font_path.c_str());
            theme::UI_FONT_LOADED = true;
        }

        // Register loaded fonts with FontLoader for P2 font listing
        fonts::FontLoader::get().loadStartupFonts(
            sophie.get<ui::FontManager>());

        {
            SCOPED_TIMER("Theme setup");
            ui::imm::ThemeDefaults::get()
                .set_theme_color(ui::Theme::Usage::Primary, colors::UI_GREEN)
                .set_theme_color(ui::Theme::Usage::Error, colors::UI_RED)
                .set_theme_color(ui::Theme::Usage::Font, colors::UI_WHITE)
                .set_theme_color(
                    ui::Theme::Usage::DarkFont,
                    afterhours::Color{30, 30, 30, 255})
                .set_theme_color(ui::Theme::Usage::Background, colors::UI_BLACK)
                .set_theme_color(
                    ui::Theme::Usage::Surface,
                    afterhours::Color{40, 40, 50, 255})
                .set_theme_color(ui::Theme::Usage::Secondary,
                                 afterhours::Color{253, 249, 0, 255})
                .set_theme_color(ui::Theme::Usage::Accent,
                                 afterhours::Color{0, 228, 48, 255});

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
    // Audio device cleanup skipped — not yet ported to Metal backend
    if (afterhours::graphics::is_window_ready()) {
        afterhours::graphics::close_window();
    }
}
