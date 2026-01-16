#pragma once

#include "renderer_interface.h"
#include "../external.h"  // For raylib types

namespace renderer {

// Raylib implementation of the renderer interface
class RaylibRenderer : public IRenderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override = default;

    // Frame management
    void beginFrame() override {
        raylib::BeginDrawing();
    }

    void endFrame() override {
        raylib::EndDrawing();
    }

    void clear(const Color& color) override {
        raylib::ClearBackground(toRaylib(color));
    }

    // Rectangle drawing
    void drawRect(const Rect& rect, const Color& color) override {
        raylib::DrawRectangleRec(toRaylib(rect), toRaylib(color));
    }

    void drawRectLines(const Rect& rect, float thickness, const Color& color) override {
        raylib::DrawRectangleLinesEx(toRaylib(rect), thickness, toRaylib(color));
    }

    void drawRectangle(int x, int y, int width, int height, const Color& color) override {
        raylib::DrawRectangle(x, y, width, height, toRaylib(color));
    }

    // Line drawing
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        raylib::DrawLine(x1, y1, x2, y2, toRaylib(color));
    }

    // Text drawing
    void drawText(const std::string& text, int x, int y, int fontSize, const Color& color) override {
        raylib::DrawText(text.c_str(), x, y, fontSize, toRaylib(color));
    }

    void drawText(const char* text, int x, int y, int fontSize, const Color& color) override {
        raylib::DrawText(text, x, y, fontSize, toRaylib(color));
    }

    // Text measurement
    int measureText(const std::string& text, int fontSize) override {
        return raylib::MeasureText(text.c_str(), fontSize);
    }

    int measureText(const char* text, int fontSize) override {
        return raylib::MeasureText(text, fontSize);
    }

    // Screen info
    int getScreenWidth() override {
        return raylib::GetScreenWidth();
    }

    int getScreenHeight() override {
        return raylib::GetScreenHeight();
    }

private:
    // Type conversion helpers
    static raylib::Color toRaylib(const Color& c) {
        return raylib::Color{c.r, c.g, c.b, c.a};
    }

    static raylib::Rectangle toRaylib(const Rect& r) {
        return raylib::Rectangle{r.x, r.y, r.width, r.height};
    }
};

// Helper to convert from raylib Color to renderer Color
inline Color fromRaylib(raylib::Color c) {
    return Color{c.r, c.g, c.b, c.a};
}

// Helper to convert from raylib Rectangle to renderer Rect
inline Rect fromRaylib(raylib::Rectangle r) {
    return Rect{r.x, r.y, r.width, r.height};
}

}  // namespace renderer
