#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Shape types for document drawings
enum class ShapeType {
    Line,         // Simple line segment
    Rectangle,    // Rectangle (can be filled or outline)
    Ellipse,      // Ellipse/circle (can be filled or outline)
    Arrow,        // Line with arrowhead
    RoundedRect,  // Rectangle with rounded corners
    Triangle,     // Triangle
    FreeformLine  // Multiple connected line segments
};

// Line style for stroke
enum class LineStyle {
    Solid,
    Dashed,
    Dotted,
    DashDot
};

// Arrow head style
enum class ArrowStyle {
    None,
    Standard,     // Classic triangle arrow
    Open,         // Open triangle (not filled)
    Diamond,      // Diamond shape
    Circle        // Circle at line end
};

// Drawing layout mode (similar to images)
enum class DrawingLayoutMode {
    Inline,     // Drawing is placed inline with text
    Float,      // Drawing floats at anchor position
    Behind,     // Drawing appears behind text
    InFront     // Drawing appears in front of text
};

// Get display name for shape type
inline const char* shapeTypeName(ShapeType type) {
    switch (type) {
        case ShapeType::Line: return "Line";
        case ShapeType::Rectangle: return "Rectangle";
        case ShapeType::Ellipse: return "Ellipse";
        case ShapeType::Arrow: return "Arrow";
        case ShapeType::RoundedRect: return "Rounded Rectangle";
        case ShapeType::Triangle: return "Triangle";
        case ShapeType::FreeformLine: return "Freeform Line";
        default: return "Shape";
    }
}

// Get display name for line style
inline const char* lineStyleName(LineStyle style) {
    switch (style) {
        case LineStyle::Solid: return "Solid";
        case LineStyle::Dashed: return "Dashed";
        case LineStyle::Dotted: return "Dotted";
        case LineStyle::DashDot: return "Dash-Dot";
        default: return "Solid";
    }
}

// Get display name for drawing layout mode
inline const char* drawingLayoutModeName(DrawingLayoutMode mode) {
    switch (mode) {
        case DrawingLayoutMode::Inline: return "Inline with Text";
        case DrawingLayoutMode::Float: return "Float";
        case DrawingLayoutMode::Behind: return "Behind Text";
        case DrawingLayoutMode::InFront: return "In Front of Text";
        default: return "Inline";
    }
}

// Color for drawings (RGBA)
struct DrawingColor {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;
    
    bool operator==(const DrawingColor& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const DrawingColor& other) const { return !(*this == other); }
    bool isTransparent() const { return a == 0; }
};

// Predefined drawing colors
namespace DrawingColors {
    constexpr DrawingColor Black = {0, 0, 0, 255};
    constexpr DrawingColor White = {255, 255, 255, 255};
    constexpr DrawingColor Red = {255, 0, 0, 255};
    constexpr DrawingColor Green = {0, 255, 0, 255};
    constexpr DrawingColor Blue = {0, 0, 255, 255};
    constexpr DrawingColor Yellow = {255, 255, 0, 255};
    constexpr DrawingColor Orange = {255, 165, 0, 255};
    constexpr DrawingColor Purple = {128, 0, 128, 255};
    constexpr DrawingColor Gray = {128, 128, 128, 255};
    constexpr DrawingColor LightGray = {192, 192, 192, 255};
    constexpr DrawingColor Transparent = {0, 0, 0, 0};
}

// Point for drawing coordinates
struct DrawingPoint {
    float x = 0.0f;
    float y = 0.0f;
    
    bool operator==(const DrawingPoint& other) const {
        return x == other.x && y == other.y;
    }
};

// Document drawing/shape data
struct DocumentDrawing {
    ShapeType shapeType = ShapeType::Line;
    
    // Position in document
    std::size_t anchorLine = 0;    // Line number where drawing is anchored
    std::size_t anchorColumn = 0;  // Column in line (for inline mode)
    
    // Drawing coordinates (relative to anchor for inline/float modes)
    // For Line/Arrow: startX/startY to endX/endY
    // For Rectangle/Ellipse: x, y, width, height
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 50.0f;
    
    // Additional points for freeform line
    std::vector<DrawingPoint> points;
    
    // Layout
    DrawingLayoutMode layoutMode = DrawingLayoutMode::Inline;
    
    // Stroke properties
    DrawingColor strokeColor = DrawingColors::Black;
    float strokeWidth = 1.0f;
    LineStyle lineStyle = LineStyle::Solid;
    
    // Fill properties
    DrawingColor fillColor = DrawingColors::Transparent;
    bool filled = false;
    
    // Arrow properties (for Arrow shape type)
    ArrowStyle startArrow = ArrowStyle::None;
    ArrowStyle endArrow = ArrowStyle::Standard;
    
    // Rounded rectangle corner radius
    float cornerRadius = 0.0f;
    
    // Rotation in degrees
    float rotation = 0.0f;
    
    // Unique identifier
    std::size_t id = 0;
    
    // Helper methods
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    
    void setSize(float w, float h) {
        width = w;
        height = h;
    }
    
    // Get bounding box
    struct Bounds {
        float x, y, width, height;
    };
    
    Bounds getBounds() const {
        return {x, y, width, height};
    }
    
    // Check if a point is inside this drawing's bounds
    bool containsPoint(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

// Drawing collection in a document
class DrawingCollection {
public:
    DrawingCollection() = default;
    
    // Add a drawing at the specified anchor position
    std::size_t addDrawing(const DocumentDrawing& drawing);
    
    // Get drawing by ID
    DocumentDrawing* getDrawing(std::size_t id);
    const DocumentDrawing* getDrawing(std::size_t id) const;
    
    // Remove drawing by ID
    bool removeDrawing(std::size_t id);
    
    // Get all drawings
    const std::vector<DocumentDrawing>& drawings() const { return drawings_; }
    std::vector<DocumentDrawing>& drawings() { return drawings_; }
    
    // Get drawings anchored at a specific line
    std::vector<DocumentDrawing*> drawingsAtLine(std::size_t line);
    std::vector<const DocumentDrawing*> drawingsAtLine(std::size_t line) const;
    
    // Get drawings in a line range
    std::vector<const DocumentDrawing*> drawingsInRange(std::size_t startLine, std::size_t endLine) const;
    
    // Update anchor positions after text edits
    void shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta);
    
    // Clear all drawings
    void clear();
    
    // Count
    std::size_t count() const { return drawings_.size(); }
    bool isEmpty() const { return drawings_.empty(); }
    
private:
    std::vector<DocumentDrawing> drawings_;
    std::size_t nextId_ = 1;
};
