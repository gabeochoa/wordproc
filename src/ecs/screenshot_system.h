#pragma once

#include "components.h"
#include "../rl.h"
#include "../../vendor/afterhours/src/core/system.h"

#include <filesystem>

namespace ecs {

// System for taking screenshots in test mode
struct ScreenshotSystem : public afterhours::System<TestConfigComponent> {
  void for_each_with(afterhours::Entity&,
                     TestConfigComponent& testConfig,
                     const float) override {
    if (!testConfig.enabled) return;
    
    testConfig.frameCount++;
    
    if (testConfig.frameCount == 1) {
      std::filesystem::create_directories(testConfig.screenshotDir);
      std::string path = testConfig.screenshotDir + "/01_startup.png";
      raylib::TakeScreenshot(path.c_str());
    }
  }
  
  // Check if we should exit (called separately in main loop)
  static bool shouldExit(const TestConfigComponent& testConfig) {
    return testConfig.enabled && 
           testConfig.frameLimit > 0 && 
           testConfig.frameCount >= testConfig.frameLimit;
  }
};

} // namespace ecs
