#include <argh.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <string>

#include "ecs/components.h"
#include "ecs/input_system.h"
#include "ecs/menu_ui_system.h"
#include "ecs/render_system.h"
#include "ecs/test_systems.h"
#include "editor/document_io.h"
#include "editor/text_buffer.h"
#include "editor/text_layout.h"
#include "input/action_map.h"
#include "preload.h"
#include "rl.h"
#include "settings.h"
#include "testing/e2e_runner.h"
#include "testing/e2e_script.h"
#include "testing/test_input.h"
#include "ui/menu_setup.h"
#include "ui/theme.h"
#include "ui/ui_context.h"
#include "ui/win95_widgets.h"
#include "util/drawing.h"
#include "util/logging.h"

// Include afterhours ECS
#include "../vendor/afterhours/src/ecs.h"

#ifdef AFTER_HOURS_ENABLE_MCP
bool g_mcp_mode = false;
int g_saved_stdout_fd = -1;
#endif

// Take a screenshot with a descriptive name
void takeScreenshot(const std::string& dir, const std::string& name) {
    // Use absolute path for screenshot
    std::filesystem::path screenshotDir = std::filesystem::absolute(dir);
    std::filesystem::create_directories(screenshotDir);
    std::filesystem::path path = screenshotDir / (name + ".png");
    raylib::TakeScreenshot(path.c_str());
}

int main(int argc, char* argv[]) {
    argh::parser cmdl(argc, argv);

    // Test mode configuration
    bool testModeEnabled = cmdl["--test-mode"];
    std::string screenshotDir = "output/screenshots";
    int frameLimit = 0;
    std::string testScriptPath;
    std::string testScriptDir;  // For batch mode
    // Parse --screenshot-dir, --frame-limit, --test-script, and --test-script-dir arguments
    // argh uses the params() map for named parameters
    for (auto& [name, value] : cmdl.params()) {
        LOG_INFO("Parsed param: %s = %s", name.c_str(), value.c_str());
        if (name == "screenshot-dir") {
            screenshotDir = value;
        } else if (name == "frame-limit") {
            frameLimit = std::stoi(value);
        } else if (name == "test-script") {
            testScriptPath = value;
        } else if (name == "test-script-dir") {
            testScriptDir = value;
        } else if (name == "e2e-debug") {
            // Value can be "true", "1", or just present
            // This is handled below after scriptRunner is set up
        }
    }
    
    // Check for e2e-debug flag (can be --e2e-debug or --e2e-debug=true)
    bool e2eDebugOverlay = cmdl["e2e-debug"] || cmdl("e2e-debug");
    LOG_INFO("screenshotDir = %s, frameLimit = %d", screenshotDir.c_str(), frameLimit);
    
    // If test script or directory is specified, enable test mode
    if (!testScriptPath.empty() || !testScriptDir.empty()) {
        testModeEnabled = true;
        test_input::test_mode = true;
    }

    // FPS test mode - simulates scrolling and logs FPS
    bool fpsTestMode = cmdl["--fps-test"];
    if (fpsTestMode) {
        testModeEnabled = true;  // FPS test implies test mode
        if (frameLimit == 0) {
            frameLimit = 120;  // Default to 120 frames for FPS test
        }
    }

    // Headless benchmark mode - measures file load time without opening window
    bool benchmarkMode = cmdl["--benchmark"];

    std::string loadFile;
    cmdl(1, "") >> loadFile;  // First positional argument is file to open

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
                          loadEnd - loadStart)
                          .count() /
                      1000.0;
        auto totalMs = std::chrono::duration_cast<std::chrono::microseconds>(
                           loadEnd - startTime)
                           .count() /
                       1000.0;

        // Get file size
        std::size_t fileSize = 0;
        if (!loadFile.empty() && std::filesystem::exists(loadFile)) {
            fileSize = std::filesystem::file_size(loadFile);
        }

        // Output CSV-friendly format using logging
        LOG_INFO(
            "file=%s,size=%zu,lines=%zu,chars=%zu,load_ms=%.3f,total_ms=%.3f,"
            "target=100,pass=%s",
            loadFile.c_str(), fileSize, buffer.lineCount(),
            buffer.getText().size(), loadMs, totalMs,
            totalMs <= 100.0 ? "true" : "false");

        return totalMs <= 100.0 ? 0 : 1;
    }

    {
        SCOPED_TIMER("Settings load");
        Settings::get().load_save_file(800, 600);
    }

    {
        SCOPED_TIMER("Preload and singletons");
        Preload::get().init("Wordproc - Untitled").make_singleton();
        Settings::get().refresh_settings();
    }

    {
        SCOPED_TIMER("UI context init");
        // Initialize Afterhours immediate-mode UI context with Win95 theme
        ui_imm::initUIContext(800, 600);
    }

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
    layoutComp.titleBarHeight =
        static_cast<float>(theme::layout::TITLE_BAR_HEIGHT);
    layoutComp.menuBarHeight =
        static_cast<float>(theme::layout::MENU_BAR_HEIGHT);
    layoutComp.statusBarHeight =
        static_cast<float>(theme::layout::STATUS_BAR_HEIGHT);
    layoutComp.borderWidth = static_cast<float>(theme::layout::BORDER_WIDTH);
    layoutComp.textPadding = static_cast<float>(theme::layout::TEXT_PADDING);

    auto& menuComp = editorEntity.addComponent<ecs::MenuComponent>();
    menuComp.menus = menu_setup::createMenuBar();

    auto& testComp = editorEntity.addComponent<ecs::TestConfigComponent>();
    testComp.enabled = testModeEnabled;
    testComp.screenshotDir = screenshotDir;
    testComp.frameLimit = frameLimit;
    testComp.fpsTestMode = fpsTestMode;

    // Setup SystemManager with all systems
    SystemManager systemManager;

    // Register pre-layout UI systems (context begin, clear children)
    ui_imm::registerUIPreLayoutSystems(systemManager);
    
    // MenuUISystem creates UI elements - must run BEFORE autolayout
    systemManager.register_update_system(
        std::make_unique<ecs::MenuUISystem>());
    
    // Register post-layout UI systems (autolayout, interactions)
    // This computes sizes for all UI elements created above
    ui_imm::registerUIPostLayoutSystems(systemManager);

    // Update systems (run every frame for input/logic)
    systemManager.register_update_system(
        std::make_unique<ecs::CaretBlinkSystem>());
    systemManager.register_update_system(
        std::make_unique<ecs::LayoutUpdateSystem>());
    systemManager.register_update_system(
        std::make_unique<ecs::TextInputSystem>());
    systemManager.register_update_system(
        std::make_unique<ecs::KeyboardShortcutSystem>());
    systemManager.register_update_system(
        std::make_unique<ecs::NavigationSystem>());

    // Render systems (run after update for drawing)
    // EditorRenderSystem must be first - it calls BeginDrawing() in once()
    systemManager.register_render_system(
        std::make_unique<ecs::EditorRenderSystem>());
    // Afterhours UI render systems (renders buttons, divs, etc.)
    ui_imm::registerUIRenderSystems(systemManager);
    // MenuSystem draws dialogs and help windows (legacy Win95 widgets)
    systemManager.register_render_system(std::make_unique<ecs::MenuSystem>());
    // Note: Screenshots are now handled in EditorRenderSystem.after() before EndDrawing()

    // Measure startup time
    auto readyTime = std::chrono::high_resolution_clock::now();
    auto startupMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         readyTime - startTime)
                         .count();

    if (testModeEnabled) {
        LOG_INFO("Startup time: %lld ms", static_cast<long long>(startupMs));
        // 500ms is realistic for a GUI app (window creation + font loading)
        if (startupMs > 500) {
            LOG_WARNING("Startup time exceeds 500ms target!");
        }
    }
    
    // Initialize E2E script runner if script specified
    e2e::ScriptRunner scriptRunner;
    if (!testScriptDir.empty()) {
        // Batch mode: load all scripts from directory (with menu/layout support)
        e2e::initializeRunnerBatch(scriptRunner, testScriptDir, docComp, menuComp, layoutComp, screenshotDir);
    } else if (!testScriptPath.empty()) {
        // Single script mode (with menu/layout support)
        e2e::initializeRunner(scriptRunner, testScriptPath, docComp, menuComp, layoutComp, screenshotDir);
    }
    
    // Enable debug overlay if requested
    if (e2eDebugOverlay) {
        scriptRunner.setDebugOverlay(true);
    }

    while (!raylib::WindowShouldClose()) {
        float dt = raylib::GetFrameTime();
        
        // Reset test input frame state
        test_input::reset_frame();
        
        // Clear visible text registry at start of frame (for E2E tests)
        test_input::clearVisibleTextRegistry();

        // FPS test mode: collect FPS data and simulate scrolling
        if (testComp.fpsTestMode && testComp.frameCount > 0) {
            // Skip first few frames (warm-up)
            if (testComp.frameCount > 5) {
                float fps = raylib::GetFPS();
                testComp.fpsSum += fps;
                testComp.fpsSamples++;
                if (fps < testComp.fpsMin) testComp.fpsMin = fps;
                if (fps > testComp.fpsMax) testComp.fpsMax = fps;
            }

            // Simulate scroll input by directly manipulating scroll offset
            auto& scrollComp = editorEntity.get<ecs::ScrollComponent>();
            // Scroll down by 3 lines each frame to simulate mouse wheel
            scrollComp.offset += 3;
            // Clamp to max scroll
            int lineCount = static_cast<int>(docComp.buffer.lineCount());
            int maxScroll = lineCount - scrollComp.visibleLines;
            if (maxScroll < 0) maxScroll = 0;
            if (scrollComp.offset > maxScroll) {
                // Wrap around to keep scrolling
                scrollComp.offset = 0;
            }
        }

        // Run all systems through the SystemManager
        systemManager.run(dt);
        
        // Execute E2E script if active (AFTER systems run so visible text is registered)
        if (scriptRunner.hasCommands() && !scriptRunner.isFinished()) {
            // Update debug overlay info in TestConfigComponent
            if (scriptRunner.showDebugOverlay()) {
                testComp.e2eDebugOverlay = true;
                testComp.e2eCurrentCommand = scriptRunner.getCurrentCommandDescription();
                testComp.e2eTimeoutSeconds = scriptRunner.getRemainingTimeoutSeconds();
            }
            
            scriptRunner.tick();
            
            // If script finished, print results and exit
            if (scriptRunner.isFinished()) {
                scriptRunner.printResults();
                takeScreenshot(screenshotDir, "final");
                
                Settings::get().write_save_file();
                return scriptRunner.hasFailed() ? 1 : 0;
            }
        } else {
            // Clear debug overlay when not running
            testComp.e2eDebugOverlay = false;
        }

        // Check for test mode exit
        if (testComp.enabled && testComp.frameLimit > 0 &&
            testComp.frameCount >= testComp.frameLimit) {
            takeScreenshot(testComp.screenshotDir, "final");

            // Output FPS test results
            if (testComp.fpsTestMode && testComp.fpsSamples > 0) {
                float avgFps =
                    testComp.fpsSum / static_cast<float>(testComp.fpsSamples);
                LOG_INFO("FPS Test Results:");
                LOG_INFO("  avg_fps=%.2f", avgFps);
                LOG_INFO("  min_fps=%.2f", testComp.fpsMin);
                LOG_INFO("  max_fps=%.2f", testComp.fpsMax);
                LOG_INFO("  samples=%d", testComp.fpsSamples);
                LOG_INFO("  file=%s", loadFile.c_str());
                LOG_INFO("  lines=%zu", docComp.buffer.lineCount());
            }

            break;
        }
    }

    Settings::get().write_save_file();
    return 0;
}
