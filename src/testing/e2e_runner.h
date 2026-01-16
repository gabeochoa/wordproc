#pragma once

#include <string>

#include "../ecs/components.h"
#include "e2e_script.h"

namespace e2e {

// Initialize the E2E script runner with a single script
void initializeRunner(
    ScriptRunner& runner,
    const std::string& scriptPath,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
);

// Initialize the E2E script runner in batch mode (loads all scripts from directory)
// Scripts are run in sequence with document cleared between each
void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
);

}  // namespace e2e

