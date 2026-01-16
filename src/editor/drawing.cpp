#include "drawing.h"

#include <algorithm>

std::size_t ShapeCollection::addShape(const DocumentShape& shape) {
    DocumentShape newShape = shape;
    newShape.id = nextId_++;
    shapes_.push_back(newShape);
    return newShape.id;
}

DocumentShape* ShapeCollection::getShape(std::size_t id) {
    for (auto& shape : shapes_) {
        if (shape.id == id) return &shape;
    }
    return nullptr;
}

const DocumentShape* ShapeCollection::getShape(std::size_t id) const {
    for (const auto& shape : shapes_) {
        if (shape.id == id) return &shape;
    }
    return nullptr;
}

bool ShapeCollection::removeShape(std::size_t id) {
    auto it = std::find_if(shapes_.begin(), shapes_.end(),
                           [id](const DocumentShape& shape) { return shape.id == id; });
    if (it != shapes_.end()) {
        shapes_.erase(it);
        return true;
    }
    return false;
}

std::vector<DocumentShape*> ShapeCollection::shapesAtLine(std::size_t line) {
    std::vector<DocumentShape*> result;
    for (auto& shape : shapes_) {
        if (shape.anchorLine == line) {
            result.push_back(&shape);
        }
    }
    return result;
}

std::vector<const DocumentShape*> ShapeCollection::shapesAtLine(std::size_t line) const {
    std::vector<const DocumentShape*> result;
    for (const auto& shape : shapes_) {
        if (shape.anchorLine == line) {
            result.push_back(&shape);
        }
    }
    return result;
}

void ShapeCollection::shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta) {
    for (auto& shape : shapes_) {
        if (shape.anchorLine >= line) {
            if (linesDelta < 0 && 
                shape.anchorLine < static_cast<std::size_t>(-linesDelta)) {
                shape.anchorLine = 0;
            } else {
                shape.anchorLine = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(shape.anchorLine) + linesDelta);
            }
        }
    }
}

void ShapeCollection::clear() {
    shapes_.clear();
    nextId_ = 1;
}

DocumentShape* ShapeCollection::shapeAtPoint(float x, float y) {
    // Search in reverse order (topmost shape first)
    for (auto it = shapes_.rbegin(); it != shapes_.rend(); ++it) {
        if (it->containsPoint(x, y)) {
            return &(*it);
        }
    }
    return nullptr;
}

const DocumentShape* ShapeCollection::shapeAtPoint(float x, float y) const {
    for (auto it = shapes_.rbegin(); it != shapes_.rend(); ++it) {
        if (it->containsPoint(x, y)) {
            return &(*it);
        }
    }
    return nullptr;
}

// Factory functions
DocumentShape createLine(Point2D start, Point2D end, float strokeWidth) {
    DocumentShape shape;
    shape.type = ShapeType::Line;
    shape.lineStart = start;
    shape.lineEnd = end;
    shape.stroke.width = strokeWidth;
    shape.stroke.style = StrokeStyle::Solid;
    shape.fill.style = FillStyle::None;
    
    // Set bounding box
    shape.position.x = std::min(start.x, end.x);
    shape.position.y = std::min(start.y, end.y);
    shape.width = std::abs(end.x - start.x);
    shape.height = std::abs(end.y - start.y);
    
    return shape;
}

DocumentShape createArrow(Point2D start, Point2D end, float strokeWidth) {
    DocumentShape shape = createLine(start, end, strokeWidth);
    shape.type = ShapeType::Arrow;
    shape.stroke.endArrow = ArrowHead::Triangle;
    return shape;
}

DocumentShape createRectangle(float x, float y, float width, float height) {
    DocumentShape shape;
    shape.type = ShapeType::Rectangle;
    shape.position = {x, y};
    shape.width = width;
    shape.height = height;
    shape.stroke.style = StrokeStyle::Solid;
    shape.stroke.width = 1.0f;
    shape.fill.style = FillStyle::None;
    return shape;
}

DocumentShape createEllipse(float x, float y, float width, float height) {
    DocumentShape shape;
    shape.type = ShapeType::Ellipse;
    shape.position = {x, y};
    shape.width = width;
    shape.height = height;
    shape.stroke.style = StrokeStyle::Solid;
    shape.stroke.width = 1.0f;
    shape.fill.style = FillStyle::None;
    return shape;
}

DocumentShape createRoundedRect(float x, float y, float width, float height, float radius) {
    DocumentShape shape;
    shape.type = ShapeType::RoundedRect;
    shape.position = {x, y};
    shape.width = width;
    shape.height = height;
    shape.cornerRadius = radius;
    shape.stroke.style = StrokeStyle::Solid;
    shape.stroke.width = 1.0f;
    shape.fill.style = FillStyle::None;
    return shape;
}
