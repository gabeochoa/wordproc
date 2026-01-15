#include "preload.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>

#include "log.h"
#include "rl.h"

#include "input_mapping.h"
#include "settings.h"
#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/files.h>
#include <afterhours/src/plugins/ui/theme.h>

using namespace afterhours;

#ifdef AFTER_HOURS_ENABLE_MCP
extern bool g_mcp_mode;

// Custom log callback that writes to stderr instead of stdout
static void mcp_trace_log_callback(int logLevel, const char *text,
                                   va_list args) {
  if (logLevel < raylib::LOG_ERROR) {
    return; // In MCP mode, only log errors
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
  files::init("Prime Pressure", "resources");

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

  raylib::InitWindow(width, height, title);
  raylib::SetWindowSize(width, height);
  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);

  raylib::SetTargetFPS(200);

  raylib::SetAudioStreamBufferSizeDefault(4096);
  raylib::InitAudioDevice();
  if (!raylib::IsAudioDeviceReady()) {
    log_warn("audio device not ready; continuing without audio");
  }
  raylib::SetMasterVolume(1.f);

  raylib::SetExitKey(0);

  load_gamepad_mappings();

  return *this;
}

// Generate codepoint array for a Unicode range
static std::vector<int> generate_codepoint_range(int start, int end) {
  std::vector<int> codepoints;
  for (int i = start; i <= end; ++i) {
    codepoints.push_back(i);
  }
  return codepoints;
}

// Build codepoints for Korean: Hangul Syllables + Hangul Jamo + ASCII
static std::vector<int> get_korean_codepoints() {
  std::vector<int> codepoints;
  // ASCII (0x0020-0x007F)
  auto ascii = generate_codepoint_range(0x0020, 0x007F);
  codepoints.insert(codepoints.end(), ascii.begin(), ascii.end());
  // Hangul Jamo (0x1100-0x11FF)
  auto jamo = generate_codepoint_range(0x1100, 0x11FF);
  codepoints.insert(codepoints.end(), jamo.begin(), jamo.end());
  // Hangul Syllables (0xAC00-0xD7AF) - most common Korean characters
  auto syllables = generate_codepoint_range(0xAC00, 0xD7AF);
  codepoints.insert(codepoints.end(), syllables.begin(), syllables.end());
  return codepoints;
}

// Build codepoints for Japanese: Hiragana + Katakana + common Kanji + ASCII
static std::vector<int> get_japanese_codepoints() {
  std::vector<int> codepoints;
  // ASCII (0x0020-0x007F)
  auto ascii = generate_codepoint_range(0x0020, 0x007F);
  codepoints.insert(codepoints.end(), ascii.begin(), ascii.end());
  // Hiragana (0x3040-0x309F)
  auto hiragana = generate_codepoint_range(0x3040, 0x309F);
  codepoints.insert(codepoints.end(), hiragana.begin(), hiragana.end());
  // Katakana (0x30A0-0x30FF)
  auto katakana = generate_codepoint_range(0x30A0, 0x30FF);
  codepoints.insert(codepoints.end(), katakana.begin(), katakana.end());
  // Common CJK Unified Ideographs subset (0x4E00-0x9FFF) - full range
  auto kanji = generate_codepoint_range(0x4E00, 0x9FFF);
  codepoints.insert(codepoints.end(), kanji.begin(), kanji.end());
  return codepoints;
}

Preload &Preload::make_singleton() {
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);

    // Load fonts for all supported languages
    std::string english_font =
        files::get_resource_path("fonts", "Gaegu-Bold.ttf").string();
    std::string korean_font =
        files::get_resource_path("fonts", "NotoSansMonoCJKkr-Bold.otf")
            .string();
    std::string japanese_font =
        files::get_resource_path("fonts", "Sazanami-Hanazono-Mincho.ttf")
            .string();
    // New fonts for game screens
    std::string rounded_font =
        files::get_resource_path("fonts", "eqprorounded-regular.ttf").string();
    std::string garamond_font =
        files::get_resource_path("fonts", "EBGaramond-Regular.ttf").string();
    std::string symbols_font =
        files::get_resource_path("fonts", "SymbolsNerdFont-Regular.ttf").string();
    // New fonts for better inspiration matching
    std::string fredoka_font =
        files::get_resource_path("fonts", "Fredoka-VariableFont_wdth,wght.ttf").string();
    std::string blackops_font =
        files::get_resource_path("fonts", "BlackOpsOne-Regular.ttf").string();
    std::string atkinson_font =
        files::get_resource_path("fonts", "AtkinsonHyperlegible-Regular.ttf").string();

    // Get codepoints for CJK fonts
    auto korean_cps = get_korean_codepoints();
    auto japanese_cps = get_japanese_codepoints();

    sophie
        .get<ui::FontManager>()
        // Default font (used when no language-specific font is set)
        .load_font(ui::UIComponent::DEFAULT_FONT, english_font.c_str())
        .load_font(ui::UIComponent::SYMBOL_FONT, english_font.c_str())
        // English font (ASCII only)
        .load_font("Gaegu-Bold", english_font.c_str())
        // Rounded font for cartoon/tycoon style
        .load_font("EqProRounded", rounded_font.c_str())
        // Garamond for elegant/cozy style
        .load_font("Garamond", garamond_font.c_str())
        // Symbols/icons font
        .load_font("NerdSymbols", symbols_font.c_str())
        // Fredoka for thick cartoon/bubble style (Tycoon, Angry Birds, Rubber Bandits)
        .load_font("Fredoka", fredoka_font.c_str())
        // Black Ops One for military/stencil style (Shooter HUD)
        .load_font("BlackOpsOne", blackops_font.c_str())
        // Atkinson Hyperlegible for accessibility
        .load_font("Atkinson", atkinson_font.c_str())
        // Korean font with Hangul codepoints
        .load_font_with_codepoints("NotoSansKR", korean_font.c_str(),
                                   korean_cps.data(),
                                   static_cast<int>(korean_cps.size()))
        // Japanese font with Hiragana/Katakana/Kanji codepoints
        .load_font_with_codepoints("Sazanami", japanese_font.c_str(),
                                   japanese_cps.data(),
                                   static_cast<int>(japanese_cps.size()));

    ui::imm::ThemeDefaults::get()
        .set_theme_color(ui::Theme::Usage::Primary, colors::UI_GREEN)
        .set_theme_color(ui::Theme::Usage::Error, colors::UI_RED)
        .set_theme_color(ui::Theme::Usage::Font, colors::UI_WHITE)
        .set_theme_color(ui::Theme::Usage::DarkFont,
                         afterhours::Color{30, 30, 30, 255}) // Dark text for light backgrounds
        .set_theme_color(ui::Theme::Usage::Background, colors::UI_BLACK)
        .set_theme_color(ui::Theme::Usage::Surface,
                         afterhours::Color{40, 40, 50, 255}) // Slightly lighter than background
        .set_theme_color(ui::Theme::Usage::Secondary, raylib::YELLOW)
        .set_theme_color(ui::Theme::Usage::Accent, raylib::GREEN);

    ui::imm::UIStylingDefaults::get().set_grid_snapping(true);

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
  if (raylib::IsAudioDeviceReady()) {
    raylib::CloseAudioDevice();
  }
  if (raylib::IsWindowReady()) {
    raylib::CloseWindow();
  }
}
