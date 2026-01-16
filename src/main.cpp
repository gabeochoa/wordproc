#include "editor/document_io.h"
#include "editor/text_buffer.h"
#include "editor/text_layout.h"
#include "ecs/components.h"
#include "ecs/input_system.h"
#include "ecs/render_system.h"
#include "ecs/test_systems.h"
#include "ecs/test_systems.h"
#include "input/action_map.h"
#include "preload.h"
#include "rl.h"
#include "settings.h"
#include "ui/menu_setup.h"
#include "ui/theme.h"
#include "ui/ui_context.h"
#include "ui/win95_widgets.h"
#include "util/drawing.h"
#include "util/logging.h"

#include <argh.h>
#include <chrono>
#include <format>
#include <filesystem>
#include <string>

// Include afterhours ECS
#include "../vendor/afterhours/src/ecs.h"

#ifdef AFTER_HOURS_ENABLE_MCP
bool g_mcp_mode = false;
int g_saved_stdout_fd = -1;
#endif

// Take a screenshot with a descriptive name
void takeScreenshot(const std::string &dir, const std::string &name) {
  std::filesystem::create_directories(dir);
  std::string path = dir + "/" + name + ".png";
  raylib::TakeScreenshot(path.c_str());
}

int main(int argc, char *argv[]) {
  argh::parser cmdl(argc, argv);
  
  // Test mode configuration
  bool testModeEnabled = cmdl["--test-mode"];
  std::string screenshotDir = "output/screenshots";
  int frameLimit = 0;
  cmdl("--screenshot-dir", "output/screenshots") >> screenshotDir;
  cmdl("--frame-limit", 0) >> frameLimit;
  
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

  // Initialize Afterhours immediate-mode UI context with Win95 theme
  ui_imm::initUIContext(800, 600);

  // Create the editor entity with all required components
  using namespace afterhours;
  
  auto& editorEntity = EntityHelper::createEntity();
  
  // Add document component
  auto& docComp = editorEntity.addComponent<ecs::DocumentComponent>();
  docComp.filePath = loadFile;
  
  // Load file if specified
  if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
    auto result = loadTextFileEx(docComp.buffer, loadFile);
    if (!result.success) {
      LOG_WARNING("Failed to load file: %s", result.error.c_str());
    }
  }
  
  // Add other components
  editorEntity.addComponent<ecs::CaretComponent>();
  editorEntity.addComponent<ecs::ScrollComponent>();
  
  editorEntity.addComponent<ecs::StatusComponent>();
  
  auto& layoutComp = editorEntity.addComponent<ecs::LayoutComponent>();
  layoutComp.titleBarHeight = static_cast<float>(theme::layout::TITLE_BAR_HEIGHT);
  layoutComp.menuBarHeight = static_cast<float>(theme::layout::MENU_BAR_HEIGHT);
  layoutComp.statusBarHeight = static_cast<float>(theme::layout::STATUS_BAR_HEIGHT);
  layoutComp.borderWidth = static_cast<float>(theme::layout::BORDER_WIDTH);
  layoutComp.textPadding = static_cast<float>(theme::layout::TEXT_PADDING);
  
  auto& menuComp = editorEntity.addComponent<ecs::MenuComponent>();
  menuComp.menus = menu_setup::createMenuBar();
  
  auto& testComp = editorEntity.addComponent<ecs::TestConfigComponent>();
  testComp.enabled = testModeEnabled;
  testComp.screenshotDir = screenshotDir;
  testComp.frameLimit = frameLimit;

  // Setup SystemManager with all systems
  SystemManager systemManager;
  
  // Register Afterhours UI systems (must be early in the update order)
  ui_imm::registerUIUpdateSystems(systemManager);
  
  // Update systems (run every frame for input/logic)
  systemManager.register_update_system(std::make_unique<ecs::CaretBlinkSystem>());
  systemManager.register_update_system(std::make_unique<ecs::LayoutUpdateSystem>());
  systemManager.register_update_system(std::make_unique<ecs::TextInputSystem>());
  systemManager.register_update_system(std::make_unique<ecs::KeyboardShortcutSystem>());
  systemManager.register_update_system(std::make_unique<ecs::NavigationSystem>());
  
  // Render systems (run after update for drawing)
  // EditorRenderSystem must be first - it calls BeginDrawing() in once()
  systemManager.register_render_system(std::make_unique<ecs::EditorRenderSystem>());
  // MenuSystem draws menus and dialogs - must run after BeginDrawing()
  systemManager.register_render_system(std::make_unique<ecs::MenuSystem>());
  systemManager.register_render_system(std::make_unique<ecs::ScreenshotSystem>());

  // Measure startup time
  auto readyTime = std::chrono::high_resolution_clock::now();
  auto startupMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      readyTime - startTime).count();
  
  if (testModeEnabled) {
    LOG_INFO("Startup time: %lld ms", static_cast<long long>(startupMs));
    if (startupMs > 100) {
      LOG_WARNING("Startup time exceeds 100ms target!");
    }
  }

  while (!raylib::WindowShouldClose()) {
    float dt = raylib::GetFrameTime();
    
    // Run all systems through the SystemManager
    systemManager.run(dt);
    
    // Check for test mode exit
    if (testComp.enabled && testComp.frameLimit > 0 && 
        testComp.frameCount >= testComp.frameLimit) {
      takeScreenshot(testComp.screenshotDir, "final");
      break;
    }
  }

  Settings::get().write_save_file();
  return 0;
}
