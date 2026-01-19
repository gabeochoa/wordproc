// E2E Runner initialization for wordproc
// Uses afterhours E2ERunner with wordproc-specific command handlers

#pragma once

#include <string>

#include "../ecs/components.h"
#include "e2e_integration.h"

namespace e2e {

// Use afterhours E2ERunner
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
