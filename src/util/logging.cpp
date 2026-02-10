#include "logging.h"

#include "../rl.h"

namespace logging {

ScopedTimer::ScopedTimer(const char* n) : name(n) {
    startTime = afterhours::graphics::get_time();
}

ScopedTimer::~ScopedTimer() {
    double elapsed = (afterhours::graphics::get_time() - startTime) * 1000.0;
    info("%s took %.3f ms", name, elapsed);
}

}  // namespace logging
