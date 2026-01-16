// E2E Runner initialization for wordproc
// Uses the extracted e2e_testing.h implementation directly

#pragma once

#include <string>

#include "../ecs/components.h"
#include "../extracted/e2e_testing.h"

namespace e2e {

// Use the extracted runner type directly
using ScriptRunner = afterhours::testing::E2ERunner;

// Initialize the E2E script runner with a single script
void initializeRunner(
    ScriptRunner& runner,
    const std::string& scriptPath,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
);

// Initialize the E2E script runner with menu support
void initializeRunner(
    ScriptRunner& runner,
    const std::string& scriptPath,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::LayoutComponent& layoutComp,
    const std::string& screenshotDir
);

// Initialize the E2E script runner in batch mode
void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
);

// Initialize the E2E script runner in batch mode with menu support
void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::LayoutComponent& layoutComp,
    const std::string& screenshotDir
);

}  // namespace e2e
