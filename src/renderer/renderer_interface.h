#pragma once

#include <string>

namespace renderer {

// Basic color type (RGBA)
struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    constexpr Color() : r(0), g(0), b(0), a(255) {}
    constexpr Color(unsigned char r_, unsigned char g_, unsigned char b_,
                    unsigned char a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}
};

// Common colors
namespace colors {
constexpr Color WHITE = {255, 255, 255, 255};
constexpr Color BLACK = {0, 0, 0, 255};
constexpr Color DARKGRAY = {80, 80, 80, 255};
constexpr Color GRAY = {128, 128, 128, 255};
constexpr Color LIGHTGRAY = {192, 192, 192, 255};
constexpr Color RED = {255, 0, 0, 255};
constexpr Color TRANSPARENT = {0, 0, 0, 0};
}  // namespace colors

// Rectangle type
struct Rect {
    float x;
    float y;
    float width;
    float height;

    constexpr Rect() : x(0), y(0), width(0), height(0) {}
    constexpr Rect(float x_, float y_, float w_, float h_)
        : x(x_), y(y_), width(w_), height(h_) {}
};

// Abstract renderer interface
// Allows swapping raylib with other renderers (SDL, OpenGL, Vulkan, etc.)
class IRenderer {
   public:
    virtual ~IRenderer() = default;

    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void clear(const Color& color) = 0;

    // Rectangle drawing
    virtual void drawRect(const Rect& rect, const Color& color) = 0;
    virtual void drawRectLines(const Rect& rect, float thickness,
                               const Color& color) = 0;
    virtual void drawRectangle(int x, int y, int width, int height,
                               const Color& color) = 0;

    // Line drawing
    virtual void drawLine(int x1, int y1, int x2, int y2,
                          const Color& color) = 0;

    // Text drawing
    virtual void drawText(const std::string& text, int x, int y, int fontSize,
                          const Color& color) = 0;
    virtual void drawText(const char* text, int x, int y, int fontSize,
                          const Color& color) = 0;

    // Text measurement
    virtual int measureText(const std::string& text, int fontSize) = 0;
    virtual int measureText(const char* text, int fontSize) = 0;

    // Screen info
    virtual int getScreenWidth() = 0;
    virtual int getScreenHeight() = 0;
};

// Global renderer accessor (set during initialization)
IRenderer& getRenderer();
void setRenderer(IRenderer* renderer);

}  // namespace renderer
