#include "renderer_interface.h"

#include <cassert>

namespace renderer {

namespace {
IRenderer* g_renderer = nullptr;
}

IRenderer& getRenderer() {
    assert(g_renderer != nullptr && "Renderer not initialized. Call setRenderer() first.");
    return *g_renderer;
}

void setRenderer(IRenderer* renderer) {
    g_renderer = renderer;
}

}  // namespace renderer
