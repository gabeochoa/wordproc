#pragma once

#include "rl.h"

namespace render_backend {
inline void BeginDrawing() { raylib::BeginDrawing(); }

inline void EndDrawing() { raylib::EndDrawing(); }

inline void BeginTextureMode(raylib::RenderTexture2D target) {
  raylib::BeginTextureMode(target);
}

inline void EndTextureMode() { raylib::EndTextureMode(); }

inline void ClearBackground(raylib::Color color) {
  raylib::ClearBackground(color);
}
} // namespace render_backend
