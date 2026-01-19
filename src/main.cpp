#include <argh.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <sstream>
#include <string>

#include "ecs/component_helpers.h"
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
#include "ui/menu_setup.h"
#include "ui/theme.h"
#include "ui/ui_context.h"
#include "ui/win95_widgets.h"
#include "util/drawing.h"
#include "util/clipboard.h"
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

// #region agent log
static void debugLog(const char* location, const char* message,
                     const char* hypothesisId, const char* runId,
                     const std::string& dataJson) {
    const char* logPath = "/Users/gabeochoa/p/wordproc/.cursor/debug.log";
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    std::ofstream out(logPath, std::ios::app);
    if (!out.is_open()) return;
    out << "{\"sessionId\":\"debug-session\",\"runId\":\"" << runId
        << "\",\"hypothesisId\":\"" << hypothesisId << "\",\"location\":\""
        << location << "\",\"message\":\"" << message << "\",\"data\":"
        << dataJson << ",\"timestamp\":" << ms << "}\n";
}
// #endregion agent log

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

    // #region agent log
    {
        std::ostringstream data;
        data << "{\"frameLimit\":" << frameLimit << ",\"testScriptPathLen\":"
             << testScriptPath.size() << ",\"testScriptDirLen\":"
             << testScriptDir.size() << ",\"testModeFlag\":"
             << (cmdl["--test-mode"] ? "true" : "false") << "}";
        debugLog("main.cpp:73", "Parsed test args", "H1", "e2e-hang-pre",
                 data.str());
    }
    // #endregion agent log
    
    // Check for e2e-debug flag (can be --e2e-debug or --e2e-debug=true)
    bool e2eDebugOverlay = cmdl["e2e-debug"] || cmdl("e2e-debug");
    LOG_INFO("screenshotDir = %s, frameLimit = %d", screenshotDir.c_str(), frameLimit);
    
    // If test script or directory is specified, enable test mode
    if (!testScriptPath.empty() || !testScriptDir.empty()) {
        testModeEnabled = true;
        test_input::test_mode = true;
        app::clipboard::enable_test_mode();  // Use in-memory clipboard
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
        
        // Initialize test input provider when in test mode
        // This registers the TestInputProvider singleton for ECS systems to query
        if (testModeEnabled) {
            ui_imm::initTestModeUI();
        }
    }

    // Create the editor entity with all required components
    using namespace afterhours;

    auto& editorEntity = EntityHelper::createEntity();

    // Add document component
    auto& docComp = editorEntity.addComponent<ecs::DocumentComponent>();
    docComp.filePath = loadFile;
    if (testModeEnabled) {
        docComp.autoSaveIntervalSeconds = 0.0;
        docComp.lastAutoSaveTime = -1.0;
    }

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

    auto& statusComp = editorEntity.addComponent<ecs::StatusComponent>();

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
    menuComp.menus = menu_setup::createMenuBar(Settings::get().get_recent_files());
    menuComp.recentFilesCount =
        static_cast<int>(Settings::get().get_recent_files().size());

    // Auto-save recovery (only when no file is explicitly opened, skip in test mode)
    if (!testModeEnabled && docComp.filePath.empty() &&
        std::filesystem::exists(docComp.autoSavePath)) {
        auto result = loadDocumentEx(docComp.buffer, docComp.docSettings,
                                     docComp.autoSavePath);
        if (result.success) {
            docComp.isDirty = true;
            ecs::status::set(statusComp, "Recovered auto-save");
            statusComp.expiresAt = raylib::GetTime() + 3.0;
        }
    }

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
        std::make_unique<ecs::AutoSaveSystem>());
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
    
    // Register E2E command handler systems if running tests
    if (scriptRunner.hasCommands()) {
        e2e::E2EConfig e2eConfig;
        e2eConfig.doc_comp = &docComp;
        e2eConfig.menu_comp = &menuComp;
        e2e::register_e2e_systems(systemManager, e2eConfig);
    }
    
    // Enable debug overlay if requested
    // Note: Debug overlay not yet supported by afterhours E2ERunner
    (void)e2eDebugOverlay;

    if ((!testScriptPath.empty() || !testScriptDir.empty()) && frameLimit == 0) {
        frameLimit = 600;  // Safety net for E2E runs (~10s at 60fps)
    }
    if (!testScriptPath.empty() && !scriptRunner.hasCommands()) {
        LOG_WARNING("E2E script has no commands: %s", testScriptPath.c_str());
        // #region agent log
        debugLog("main.cpp:286", "No commands loaded for script", "H2",
                 "e2e-hang-pre", "{\"hasCommands\":false}");
        // #endregion agent log
        return 1;
    }
    if (!testScriptDir.empty() && !scriptRunner.hasCommands()) {
        LOG_WARNING("E2E script directory has no commands: %s", testScriptDir.c_str());
        // #region agent log
        debugLog("main.cpp:293", "No commands loaded for directory", "H2",
                 "e2e-hang-pre", "{\"hasCommands\":false}");
        // #endregion agent log
        return 1;
    }

    // #region agent log
    {
        std::ostringstream data;
        data << "{\"hasCommands\":" << (scriptRunner.hasCommands() ? "true" : "false")
             << ",\"frameLimit\":" << frameLimit
             << ",\"testScriptPath\":\"" << testScriptPath
             << "\",\"testScriptDir\":\"" << testScriptDir
             << "\",\"screenshotDir\":\"" << screenshotDir << "\"}";
        debugLog("main.cpp:301", "Runner initialized", "H5", "e2e-hang-suite",
                 data.str());
    }
    // #endregion agent log

    int loopFrames = 0;
    const bool e2eActive = !testScriptPath.empty() || !testScriptDir.empty();
    const auto e2eStartTime = std::chrono::steady_clock::now();
    while (!raylib::WindowShouldClose()) {
        float dt = raylib::GetFrameTime();
        loopFrames++;
        
        // Reset test input frame state (but keep mouse state from pending simulation)
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
        
        // Execute E2E script AFTER systems run (visible text is now registered for validation)
        if (scriptRunner.hasCommands() && !scriptRunner.isFinished()) {
            // Note: Debug overlay not yet supported by afterhours E2ERunner
            // testComp.e2eDebugOverlay would be set here if supported
            
            scriptRunner.tick();
            
            // If script finished, print results and exit
            if (scriptRunner.isFinished()) {
                // #region agent log
                debugLog("main.cpp:404", "Script finished", "H3",
                         "e2e-hang-pre",
                         std::string("{\"hasFailed\":") +
                             (scriptRunner.hasFailed() ? "true" : "false") +
                             "}");
                // #endregion agent log
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
            loopFrames >= testComp.frameLimit) {
            // #region agent log
            {
                std::ostringstream data;
                data << "{\"loopFrames\":" << loopFrames
                     << ",\"frameLimit\":" << testComp.frameLimit
                     << ",\"scriptFinished\":"
                     << (scriptRunner.isFinished() ? "true" : "false") << "}";
                debugLog("main.cpp:418", "Frame limit reached", "H1",
                         "e2e-hang-pre", data.str());
            }
            // #endregion agent log
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

        if (e2eActive) {
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - e2eStartTime)
                    .count();
            if (elapsed > 8) {
                LOG_WARNING("E2E timeout after %lld seconds",
                            static_cast<long long>(elapsed));
                // #region agent log
                {
                    std::ostringstream data;
                    data << "{\"elapsedSeconds\":" << elapsed
                         << ",\"loopFrames\":" << loopFrames
                         << ",\"scriptFinished\":"
                         << (scriptRunner.isFinished() ? "true" : "false")
                         << "}";
                    debugLog("main.cpp:442", "E2E timeout", "H4",
                             "e2e-hang-pre", data.str());
                }
                // #endregion agent log
                takeScreenshot(testComp.screenshotDir, "final");
                break;
            }
        }
    }

    Settings::get().write_save_file();
    return 0;
}
