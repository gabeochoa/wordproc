#include "logging.h"

#include "../rl.h"

namespace logging {

ScopedTimer::ScopedTimer(const char* n) : name(n) {
    startTime = raylib::GetTime();
}

ScopedTimer::~ScopedTimer() {
    double elapsed = (raylib::GetTime() - startTime) * 1000.0;
    info("%s took %.3f ms", name, elapsed);
}

}  // namespace logging
