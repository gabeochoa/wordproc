#include <argh.h>

#include <chrono>
#include <filesystem>
#include <future>
#include <string>

#include "ecs/component_helpers.h"
#include "ecs/components.h"
#include "ecs/input_system.h"
#include "ecs/menu_ui_system.h"
#include "ecs/render_system.h"
#include "ecs/test_systems.h"
#include "ecs/toolbar_system.h"
#include "ecs/toolbar_overlay_render.h"
#include "ecs/status_bar_system.h"
#include "ecs/title_bar_system.h"
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
#include <afterhours/src/plugins/ui/validation_systems.h>
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

// ── Background file preloading ──
// Loads the document on a background thread while Sokol/Metal creates the window.
// The ~300ms window creation time is otherwise dead CPU time.
struct PreloadedDocument {
    TextBuffer buffer;
    DocumentSettings docSettings;
    bool loaded = false;
    std::string error;
};

// ── Shared state between main() and the run() callbacks ──
namespace app_state {

// Command-line config (set before run())
bool testModeEnabled = false;
std::string screenshotDir = "tests/screenshots";
int frameLimit = 0;
std::string testScriptPath;
std::string testScriptDir;
float e2eTimeout = 30.0f;
bool e2eDebugOverlay = false;
bool fpsTestMode = false;
bool benchmarkMode = false;
std::string loadFile;

// Background document preload (launched before sapp_run, collected in app_init)
std::future<PreloadedDocument> preloadFuture;

// Runtime state (used during frame callback)
afterhours::SystemManager* systemManager = nullptr;
afterhours::Entity* editorEntity = nullptr;
ecs::DocumentComponent* docComp = nullptr;
ecs::MenuComponent* menuComp = nullptr;
ecs::LayoutComponent* layoutComp = nullptr;
ecs::ToolbarComponent* toolbarComp = nullptr;
ecs::TestConfigComponent* testComp = nullptr;
ecs::ScrollComponent* scrollComp = nullptr;
e2e::ScriptRunner* scriptRunner = nullptr;

int loopFrames = 0;
bool e2eActive = false;
std::chrono::steady_clock::time_point e2eStartTime;
std::chrono::high_resolution_clock::time_point startTime;

// Return code (set by frame callback, read after run())
int returnCode = 0;
bool earlyExit = false;

}  // namespace app_state

// ── Init callback: runs after Sokol/Metal window is created ──
static void app_init() {
    using namespace afterhours;

    {
        SCOPED_TIMER("Preload and singletons");
        Preload::get().init("Wordproc - Untitled").make_singleton();
        Settings::get().refresh_settings();
    }

    {
        SCOPED_TIMER("UI context init");
        {
            SCOPED_TIMER("  initUIContext");
            ui_imm::initUIContext(
                Settings::get().get_screen_width(),
                Settings::get().get_screen_height());
        }

        {
            SCOPED_TIMER("  UIStylingDefaults setup");
            afterhours::ui::imm::UIStylingDefaults::get()
                .set_default_font(afterhours::ui::UIComponent::DEFAULT_FONT,
                                  afterhours::ui::pixels(14.0f));

            if (!app_state::testModeEnabled) {
                afterhours::ui::imm::UIStylingDefaults::get().enable_development_validation();
            }
        }

        if (app_state::testModeEnabled) {
            SCOPED_TIMER("  initTestModeUI");
            ui_imm::initTestModeUI();
        }
    }

    // Create the editor entity with all required components
    auto& editorEntity = EntityHelper::createEntity();
    app_state::editorEntity = &editorEntity;

    SCOPED_TIMER("Entity + component setup");
    // Add document component
    auto& docComp = editorEntity.addComponent<ecs::DocumentComponent>();
    app_state::docComp = &docComp;
    docComp.filePath = app_state::loadFile;
    if (app_state::testModeEnabled) {
        docComp.autoSaveEnabled = false;
    }

    // Collect pre-loaded document from background thread.
    // The background thread was launched in main() before sapp_run(), so it has
    // had ~300ms of window-creation time to finish. This should be instant.
    if (app_state::preloadFuture.valid()) {
        SCOPED_TIMER("Collect preloaded document");
        auto preloaded = app_state::preloadFuture.get();
        if (preloaded.loaded) {
            docComp.buffer = std::move(preloaded.buffer);
            docComp.docSettings = std::move(preloaded.docSettings);
        } else if (!preloaded.error.empty()) {
            LOG_WARNING("Failed to load file: %s", preloaded.error.c_str());
        }
    } else if (!app_state::loadFile.empty() && std::filesystem::exists(app_state::loadFile)) {
        // Fallback: synchronous load (e.g., benchmark mode or no file specified)
        SCOPED_TIMER("Load document file (sync fallback)");
        auto result = loadTextFileEx(docComp.buffer, app_state::loadFile);
        if (!result.success) {
            LOG_WARNING("Failed to load file: %s", result.error.c_str());
        }
    }

    // Add other components
    editorEntity.addComponent<ecs::CaretComponent>();
    app_state::scrollComp = &editorEntity.addComponent<ecs::ScrollComponent>();

    // StatusComponent kept for status bar text (word count, etc.)
    editorEntity.addComponent<ecs::StatusComponent>();

    auto& layoutComp = editorEntity.addComponent<ecs::LayoutComponent>();
    app_state::layoutComp = &layoutComp;
    layoutComp.titleBarHeight =
        static_cast<float>(theme::layout::TITLE_BAR_HEIGHT);
    layoutComp.menuBarHeight =
        static_cast<float>(theme::layout::MENU_BAR_HEIGHT);
    layoutComp.statusBarHeight =
        static_cast<float>(theme::layout::STATUS_BAR_HEIGHT);
    layoutComp.borderWidth = static_cast<float>(theme::layout::BORDER_WIDTH);
    layoutComp.textPadding = static_cast<float>(theme::layout::TEXT_PADDING);

    auto& menuComp = editorEntity.addComponent<ecs::MenuComponent>();
    app_state::menuComp = &menuComp;
    {
        SCOPED_TIMER("createMenuBar");
        menuComp.menus = menu_setup::createMenuBar(Settings::get().get_recent_files());
    }
    menuComp.recentFilesCount =
        static_cast<int>(Settings::get().get_recent_files().size());

    // Auto-save recovery (only when no file is explicitly opened, skip in test mode)
    // Note: Toast notification is deferred until after systems are registered
    bool recoveredAutoSave = false;
    if (!app_state::testModeEnabled && docComp.filePath.empty() &&
        std::filesystem::exists(docComp.autoSavePath)) {
        auto result = loadDocumentEx(docComp.buffer, docComp.docSettings,
                                     docComp.autoSavePath);
        if (result.success) {
            docComp.isDirty = true;
            recoveredAutoSave = true;
        }
    }

    auto& testComp = editorEntity.addComponent<ecs::TestConfigComponent>();
    app_state::testComp = &testComp;
    testComp.enabled = app_state::testModeEnabled;
    testComp.screenshotDir = app_state::screenshotDir;
    testComp.frameLimit = app_state::frameLimit;
    testComp.fpsTestMode = app_state::fpsTestMode;

    // Add toolbar component
    auto& toolbarComp = editorEntity.addComponent<ecs::ToolbarComponent>();
    app_state::toolbarComp = &toolbarComp;
    (void)toolbarComp;  // Initialize with defaults

    // Setup SystemManager with all systems
    static SystemManager sm;
    app_state::systemManager = &sm;

    {
        SCOPED_TIMER("Register all systems");
        // Register pre-layout UI systems (context begin, clear children)
        ui_imm::registerUIPreLayoutSystems(sm);
        
        // UI-creating systems must run BETWEEN pre-layout and post-layout
        // so their entities are included in BuildUIEntityMapping and RunAutoLayout
        sm.register_update_system(
            std::make_unique<ecs::MenuUISystem>());
        sm.register_update_system(
            std::make_unique<ecs::ToolbarRenderSystem>());
        sm.register_update_system(
            std::make_unique<ecs::StatusBarSystem>());
        sm.register_update_system(
            std::make_unique<ecs::TitleBarSystem>());
        
        // Register post-layout UI systems (entity mapping, autolayout, interactions)
        // This builds the mapping and computes sizes for all UI elements created above
        ui_imm::registerUIPostLayoutSystems(sm);

        // Update systems (run every frame for input/logic)
        sm.register_update_system(
            std::make_unique<ecs::CaretBlinkSystem>());
        sm.register_update_system(
            std::make_unique<ecs::LayoutUpdateSystem>());
        sm.register_update_system(
            std::make_unique<ecs::TextInputSystem>());
        sm.register_update_system(
            std::make_unique<ecs::KeyboardShortcutSystem>());
        sm.register_update_system(
            std::make_unique<ecs::AutoSaveSystem>());
        sm.register_update_system(
            std::make_unique<ecs::NavigationSystem>());
        
        // Toast notification systems (update and layout)
        ui_imm::registerToastSystems(sm);
        
        // Modal dialog systems (input blocking, focus trapping)
        ui_imm::registerModalSystems(sm);

        // Render systems (run after update for drawing)
        // EditorRenderSystem must be first - it calls BeginDrawing() in once()
        sm.register_render_system(
            std::make_unique<ecs::EditorRenderSystem>());
        // Afterhours UI render systems (renders buttons, divs, etc.)
        ui_imm::registerUIRenderSystems(sm);
        // Toolbar icon overlays and dropdown triangles (drawn AFTER afterhours UI)
        sm.register_render_system(
            std::make_unique<ecs::ToolbarOverlayRenderSystem>());
        // Modal backdrop rendering (draws dimmed overlay behind modals)
        ui_imm::registerModalRenderSystems(sm);
        // MenuSystem draws dialogs and help windows (legacy Win95 widgets)
        sm.register_render_system(std::make_unique<ecs::MenuSystem>());

        // Validation systems (log warnings for layout/accessibility violations)
        // Only register in non-test mode to avoid slowing E2E tests
        if (!app_state::testModeEnabled) {
            afterhours::ui::validation::register_systems<InputAction>(sm);
        }
    }

    // Measure startup time
    auto readyTime = std::chrono::high_resolution_clock::now();
    auto startupMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         readyTime - app_state::startTime)
                         .count();

    if (app_state::testModeEnabled) {
        LOG_INFO("Startup time: %lld ms", static_cast<long long>(startupMs));
        // 500ms is realistic for a GUI app (window creation + font loading)
        if (startupMs > 500) {
            LOG_WARNING("Startup time exceeds 500ms target!");
        }
    }
    
    // Initialize E2E script runner if script specified
    static e2e::ScriptRunner runner;
    app_state::scriptRunner = &runner;
    if (!app_state::testScriptDir.empty()) {
        // Batch mode: load all scripts from directory
        e2e::initializeRunnerBatch(runner, app_state::testScriptDir, docComp, menuComp, layoutComp, toolbarComp, app_state::screenshotDir);
    } else if (!app_state::testScriptPath.empty()) {
        // Single script mode
        e2e::initializeRunner(runner, app_state::testScriptPath, docComp, menuComp, layoutComp, toolbarComp, app_state::screenshotDir);
    }
    
    // Set E2E timeout (default 30s, can be increased for large document tests)
    runner.set_timeout(app_state::e2eTimeout);
    
    // Register E2E command handler systems if running tests
    if (runner.hasCommands()) {
        e2e::E2EConfig e2eConfig;
        e2eConfig.doc_comp = &docComp;
        e2eConfig.menu_comp = &menuComp;
        e2eConfig.screenshot_callback = [](const std::string& name) {
            std::filesystem::path dir = std::filesystem::absolute(app_state::screenshotDir);
            std::filesystem::create_directories(dir);
            std::filesystem::path path = dir / (name + ".png");
            afterhours::graphics::take_screenshot(path.c_str());
        };
        e2e::register_e2e_systems(sm, e2eConfig);
    }
    
    // Enable debug overlay if requested
    // Note: Debug overlay not yet supported by afterhours E2ERunner
    (void)app_state::e2eDebugOverlay;

    if ((!app_state::testScriptPath.empty() || !app_state::testScriptDir.empty()) && app_state::frameLimit == 0) {
        // Calculate frame limit from timeout (60 fps * timeout seconds, with some buffer)
        app_state::frameLimit = static_cast<int>(app_state::e2eTimeout * 60 * 1.5f);
        testComp.frameLimit = app_state::frameLimit;
    }
    if (!app_state::testScriptPath.empty() && !runner.hasCommands()) {
        LOG_WARNING("E2E script has no commands: %s", app_state::testScriptPath.c_str());
        app_state::returnCode = 1;
        app_state::earlyExit = true;
        afterhours::graphics::request_quit();
        return;
    }
    if (!app_state::testScriptDir.empty() && !runner.hasCommands()) {
        LOG_WARNING("E2E script directory has no commands: %s", app_state::testScriptDir.c_str());
        app_state::returnCode = 1;
        app_state::earlyExit = true;
        afterhours::graphics::request_quit();
        return;
    }

    // Send deferred toast notification for auto-save recovery
    // (must happen after toast systems are registered)
    if (recoveredAutoSave) {
        toast_notify::info("[recovered] Restored from auto-save", 5.0f);
    }

    app_state::e2eActive = !app_state::testScriptPath.empty() || !app_state::testScriptDir.empty();
    app_state::e2eStartTime = std::chrono::steady_clock::now();
}

// ── Frame callback: runs every frame ──
static void app_frame() {
    if (app_state::earlyExit) return;

    float dt = afterhours::graphics::get_frame_time();
    app_state::loopFrames++;
    
    // Reset test input frame state (but keep mouse state from pending simulation)
    test_input::reset_frame();
    
    // Auto-release mouse after click to prevent stuck-down state.
    // simulate_click() sets left_down=true but never releases it, which means
    // subsequent clicks can't trigger just_pressed (prev_mouse_down is already true).
    // We detect the press->down transition and release after 2 frames so that
    // BeginUIContextManager sees left_down=true for at least one full frame.
    {
        static bool prev_left_down = false;
        static int release_countdown = -1;
        auto& m = afterhours::testing::input_injector::detail::mouse;
        
        // Detect new press (transition from not-down to down)
        if (m.left_down && !prev_left_down) {
            release_countdown = 2; // Release 2 frames after press detected
        }
        prev_left_down = m.left_down;
        
        // Count down and release
        if (release_countdown > 0) {
            release_countdown--;
        } else if (release_countdown == 0) {
            release_countdown = -1;
            if (m.left_down) {
                m.left_down = false;
                m.just_released = true;
            }
            prev_left_down = false;
        }
    }
    
    // Clear visible text registry at start of frame (for E2E tests)
    test_input::clear_visible_text_registry();

    auto* testComp = app_state::testComp;

    // FPS test mode: collect FPS data and simulate scrolling
    if (testComp->fpsTestMode && testComp->frameCount > 0) {
        // Skip first few frames (warm-up)
        if (testComp->frameCount > 5) {
            float fps = afterhours::graphics::get_fps();
            testComp->fpsSum += fps;
            testComp->fpsSamples++;
            if (fps < testComp->fpsMin) testComp->fpsMin = fps;
            if (fps > testComp->fpsMax) testComp->fpsMax = fps;
        }

        // Simulate scroll input by directly manipulating scroll offset
        auto* scrollComp = app_state::scrollComp;
        // Scroll down by 3 lines each frame to simulate mouse wheel
        scrollComp->offset += 3;
        // Clamp to max scroll
        int lineCount = static_cast<int>(app_state::docComp->buffer.lineCount());
        int maxScroll = lineCount - scrollComp->visibleLines;
        if (maxScroll < 0) maxScroll = 0;
        if (scrollComp->offset > maxScroll) {
            // Wrap around to keep scrolling
            scrollComp->offset = 0;
        }
    }

    // Begin the GPU render pass — all draw calls (from systems, UI, overlays)
    // must happen between begin_drawing() and end_drawing().
    afterhours::graphics::begin_drawing();
    afterhours::graphics::clear_background(theme::WINDOW_BG);

    // Run all systems through the SystemManager
    // This includes EditorRenderSystem (document/ruler/text) and
    // RenderImm (UI buttons, divs, title bar, etc.)
    app_state::systemManager->run(dt);
    
    // Execute E2E script AFTER systems run (visible text is now registered for validation)
    auto* runner = app_state::scriptRunner;
    if (runner->hasCommands() && !runner->isFinished()) {
        runner->tick();
        
        // If script finished, print results and exit
        if (runner->isFinished()) {
            runner->printResults();
            Settings::get().write_save_file();
            app_state::returnCode = runner->hasFailed() ? 1 : 0;
            afterhours::graphics::end_drawing();
            afterhours::graphics::request_quit();
            return;
        }
    } else {
        // Clear debug overlay when not running
        testComp->e2eDebugOverlay = false;
    }

    // End the GPU render pass — flush all accumulated draw commands
    afterhours::graphics::end_drawing();

    // Check for test mode exit
    if (testComp->enabled && testComp->frameLimit > 0 &&
        app_state::loopFrames >= testComp->frameLimit) {
        // Output FPS test results
        if (testComp->fpsTestMode && testComp->fpsSamples > 0) {
            float avgFps =
                testComp->fpsSum / static_cast<float>(testComp->fpsSamples);
            LOG_INFO("FPS Test Results:");
            LOG_INFO("  avg_fps=%.2f", avgFps);
            LOG_INFO("  min_fps=%.2f", testComp->fpsMin);
            LOG_INFO("  max_fps=%.2f", testComp->fpsMax);
            LOG_INFO("  samples=%d", testComp->fpsSamples);
            LOG_INFO("  file=%s", app_state::loadFile.c_str());
            LOG_INFO("  lines=%zu", app_state::docComp->buffer.lineCount());
        }

        afterhours::graphics::request_quit();
        return;
    }

    if (app_state::e2eActive) {
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - app_state::e2eStartTime)
                .count();
        // Use e2eTimeout - 2 seconds to allow runner to timeout gracefully first
        if (elapsed > static_cast<long long>(app_state::e2eTimeout - 2.0f)) {
            LOG_WARNING("E2E timeout after %lld seconds",
                        static_cast<long long>(elapsed));
            afterhours::graphics::request_quit();
            return;
        }
    }
}

// ── Cleanup callback: runs when window is closing ──
static void app_cleanup() {
    Settings::get().write_save_file();
}

int main(int argc, char* argv[]) {
    argh::parser cmdl(argc, argv);

    // Test mode configuration
    app_state::testModeEnabled = cmdl["--test-mode"];
    // Parse named parameters
    for (auto& [name, value] : cmdl.params()) {
        if (name == "screenshot-dir") {
            app_state::screenshotDir = value;
        } else if (name == "frame-limit") {
            app_state::frameLimit = std::stoi(value);
        } else if (name == "test-script") {
            app_state::testScriptPath = value;
        } else if (name == "test-script-dir") {
            app_state::testScriptDir = value;
        } else if (name == "e2e-timeout") {
            app_state::e2eTimeout = std::stof(value);
        } else if (name == "e2e-debug") {
            // Value can be "true", "1", or just present
            // This is handled below after scriptRunner is set up
        }
    }

    // Check for e2e-debug flag (can be --e2e-debug or --e2e-debug=true)
    app_state::e2eDebugOverlay = cmdl["e2e-debug"] || cmdl("e2e-debug");
    LOG_INFO("screenshotDir = %s, frameLimit = %d", app_state::screenshotDir.c_str(), app_state::frameLimit);
    
    // If test script or directory is specified, enable test mode
    if (!app_state::testScriptPath.empty() || !app_state::testScriptDir.empty()) {
        app_state::testModeEnabled = true;
        test_input::test_mode = true;
        app::clipboard::enable_test_mode();  // Use in-memory clipboard
    }

    // FPS test mode - simulates scrolling and logs FPS
    app_state::fpsTestMode = cmdl["--fps-test"];
    if (app_state::fpsTestMode) {
        app_state::testModeEnabled = true;  // FPS test implies test mode
        if (app_state::frameLimit == 0) {
            app_state::frameLimit = 120;  // Default to 120 frames for FPS test
        }
    }

    // Headless benchmark mode - measures file load time without opening window
    app_state::benchmarkMode = cmdl["--benchmark"];

    cmdl(1, "") >> app_state::loadFile;  // First positional argument is file to open

    // Track startup time
    app_state::startTime = std::chrono::high_resolution_clock::now();

    // Launch background document loading ASAP.
    // sapp_run() will block for ~300ms creating the Metal window — we use that
    // dead time to read the file, parse JSON, and build the TextBuffer.
    // By the time app_init() fires, the document is already in memory.
    if (!app_state::benchmarkMode && !app_state::loadFile.empty() &&
        std::filesystem::exists(app_state::loadFile)) {
        LOG_INFO("Launching background document preload: %s", app_state::loadFile.c_str());
        app_state::preloadFuture = std::async(std::launch::async, [filePath = app_state::loadFile]() {
            PreloadedDocument data;
            SCOPED_TIMER("Background document preload");
            auto result = loadDocumentEx(data.buffer, data.docSettings, filePath);
            data.loaded = result.success;
            data.error = result.error;
            return data;
        });
    }

    // Headless benchmark: just load file and report timing
    if (app_state::benchmarkMode) {
        TextBuffer buffer;

        auto loadStart = std::chrono::high_resolution_clock::now();

        if (!app_state::loadFile.empty() && std::filesystem::exists(app_state::loadFile)) {
            loadTextFile(buffer, app_state::loadFile);
        }

        auto loadEnd = std::chrono::high_resolution_clock::now();
        auto loadMs = std::chrono::duration_cast<std::chrono::microseconds>(
                          loadEnd - loadStart)
                          .count() /
                      1000.0;
        auto totalMs = std::chrono::duration_cast<std::chrono::microseconds>(
                           loadEnd - app_state::startTime)
                           .count() /
                       1000.0;

        // Get file size
        std::size_t fileSize = 0;
        if (!app_state::loadFile.empty() && std::filesystem::exists(app_state::loadFile)) {
            fileSize = std::filesystem::file_size(app_state::loadFile);
        }

        // Output CSV-friendly format using logging
        LOG_INFO(
            "file=%s,size=%zu,lines=%zu,chars=%zu,load_ms=%.3f,total_ms=%.3f,"
            "target=100,pass=%s",
            app_state::loadFile.c_str(), fileSize, buffer.lineCount(),
            buffer.textSize(), loadMs, totalMs,
            totalMs <= 100.0 ? "true" : "false");

        return totalMs <= 100.0 ? 0 : 1;
    }

    {
        SCOPED_TIMER("Settings load");
        Settings::get().load_save_file(800, 600);
    }

    // Launch the application via the unified run() API.
    // On Metal/Sokol, sapp_run() blocks and owns the event loop.
    afterhours::graphics::RunConfig cfg;
    cfg.width = Settings::get().get_screen_width();
    cfg.height = Settings::get().get_screen_height();
    cfg.title = "Wordproc - Untitled";
    cfg.target_fps = 200;
    cfg.flags = afterhours::graphics::FLAG_WINDOW_RESIZABLE;
    cfg.init = app_init;
    cfg.frame = app_frame;
    cfg.cleanup = app_cleanup;

    afterhours::graphics::run(cfg);

    return app_state::returnCode;
}
