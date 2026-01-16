#include "../src/editor/drawing.h"
#include "catch2/catch.hpp"

TEST_CASE("DocumentShape initialization", "[drawing]") {
    DocumentShape shape;
    
    SECTION("default values") {
        REQUIRE(shape.type == ShapeType::Rectangle);
        REQUIRE(shape.position.x == 0.0f);
        REQUIRE(shape.position.y == 0.0f);
        REQUIRE(shape.width == 100.0f);
        REQUIRE(shape.height == 100.0f);
        REQUIRE(shape.rotation == 0.0f);
    }

    SECTION("default stroke properties") {
        REQUIRE(shape.stroke.style == StrokeStyle::Solid);
        REQUIRE(shape.stroke.width == 1.0f);
        REQUIRE(shape.stroke.lineCap == LineCap::Flat);
        REQUIRE(shape.stroke.startArrow == ArrowHead::None);
        REQUIRE(shape.stroke.endArrow == ArrowHead::None);
    }

    SECTION("default fill properties") {
        REQUIRE(shape.fill.style == FillStyle::None);
        REQUIRE(shape.fill.opacity == 255);
    }
}

TEST_CASE("DocumentShape bounds and containment", "[drawing]") {
    DocumentShape shape;
    shape.position = {10.0f, 20.0f};
    shape.width = 100.0f;
    shape.height = 50.0f;

    SECTION("getBounds returns correct values") {
        auto bounds = shape.getBounds();
        REQUIRE(bounds.x == 10.0f);
        REQUIRE(bounds.y == 20.0f);
        REQUIRE(bounds.width == 100.0f);
        REQUIRE(bounds.height == 50.0f);
    }

    SECTION("getBounds includes offset") {
        shape.offsetX = 5.0f;
        shape.offsetY = -10.0f;
        auto bounds = shape.getBounds();
        REQUIRE(bounds.x == 15.0f);
        REQUIRE(bounds.y == 10.0f);
    }

    SECTION("containsPoint inside shape") {
        REQUIRE(shape.containsPoint(50.0f, 40.0f));
        REQUIRE(shape.containsPoint(10.0f, 20.0f));  // Corner
        REQUIRE(shape.containsPoint(110.0f, 70.0f)); // Opposite corner
    }

    SECTION("containsPoint outside shape") {
        REQUIRE_FALSE(shape.containsPoint(5.0f, 40.0f));   // Left
        REQUIRE_FALSE(shape.containsPoint(115.0f, 40.0f)); // Right
        REQUIRE_FALSE(shape.containsPoint(50.0f, 10.0f));  // Above
        REQUIRE_FALSE(shape.containsPoint(50.0f, 80.0f));  // Below
    }
}

TEST_CASE("Point2D", "[drawing]") {
    SECTION("default constructor") {
        Point2D p;
        REQUIRE(p.x == 0.0f);
        REQUIRE(p.y == 0.0f);
    }

    SECTION("parameterized constructor") {
        Point2D p(10.0f, 20.0f);
        REQUIRE(p.x == 10.0f);
        REQUIRE(p.y == 20.0f);
    }

    SECTION("equality comparison") {
        Point2D a(5.0f, 10.0f);
        Point2D b(5.0f, 10.0f);
        Point2D c(5.0f, 11.0f);
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
    }
}

TEST_CASE("ShapeType names", "[drawing]") {
    SECTION("all types have display names") {
        REQUIRE(std::string(shapeTypeName(ShapeType::Line)) == "Line");
        REQUIRE(std::string(shapeTypeName(ShapeType::Arrow)) == "Arrow");
        REQUIRE(std::string(shapeTypeName(ShapeType::Rectangle)) == "Rectangle");
        REQUIRE(std::string(shapeTypeName(ShapeType::RoundedRect)) == "Rounded Rectangle");
        REQUIRE(std::string(shapeTypeName(ShapeType::Ellipse)) == "Ellipse");
        REQUIRE(std::string(shapeTypeName(ShapeType::Triangle)) == "Triangle");
        REQUIRE(std::string(shapeTypeName(ShapeType::Diamond)) == "Diamond");
        REQUIRE(std::string(shapeTypeName(ShapeType::Pentagon)) == "Pentagon");
        REQUIRE(std::string(shapeTypeName(ShapeType::Hexagon)) == "Hexagon");
        REQUIRE(std::string(shapeTypeName(ShapeType::Star)) == "Star");
        REQUIRE(std::string(shapeTypeName(ShapeType::Callout)) == "Callout");
        REQUIRE(std::string(shapeTypeName(ShapeType::Bracket)) == "Bracket");
        REQUIRE(std::string(shapeTypeName(ShapeType::Freeform)) == "Freeform");
    }
}

TEST_CASE("ShapeCollection initialization", "[drawing]") {
    ShapeCollection collection;
    
    SECTION("starts empty") {
        REQUIRE(collection.isEmpty());
        REQUIRE(collection.count() == 0);
        REQUIRE(collection.shapes().empty());
    }
}

TEST_CASE("ShapeCollection add and get", "[drawing]") {
    ShapeCollection collection;
    
    SECTION("addShape assigns unique IDs") {
        DocumentShape s1;
        s1.type = ShapeType::Rectangle;
        std::size_t id1 = collection.addShape(s1);
        
        DocumentShape s2;
        s2.type = ShapeType::Ellipse;
        std::size_t id2 = collection.addShape(s2);
        
        REQUIRE(id1 != id2);
        REQUIRE(collection.count() == 2);
    }

    SECTION("getShape retrieves by ID") {
        DocumentShape s;
        s.type = ShapeType::Line;
        std::size_t id = collection.addShape(s);
        
        DocumentShape* retrieved = collection.getShape(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->type == ShapeType::Line);
    }

    SECTION("getShape returns nullptr for invalid ID") {
        REQUIRE(collection.getShape(999) == nullptr);
    }

    SECTION("const getShape works") {
        DocumentShape s;
        s.type = ShapeType::Arrow;
        std::size_t id = collection.addShape(s);
        
        const ShapeCollection& constCollection = collection;
        const DocumentShape* retrieved = constCollection.getShape(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->type == ShapeType::Arrow);
    }
}

TEST_CASE("ShapeCollection remove", "[drawing]") {
    ShapeCollection collection;
    DocumentShape s;
    std::size_t id = collection.addShape(s);

    SECTION("removeShape removes existing shape") {
        REQUIRE(collection.count() == 1);
        bool removed = collection.removeShape(id);
        REQUIRE(removed);
        REQUIRE(collection.count() == 0);
        REQUIRE(collection.getShape(id) == nullptr);
    }

    SECTION("removeShape returns false for non-existent ID") {
        bool removed = collection.removeShape(999);
        REQUIRE_FALSE(removed);
        REQUIRE(collection.count() == 1);
    }
}

TEST_CASE("ShapeCollection shapesAtLine", "[drawing]") {
    ShapeCollection collection;
    
    DocumentShape s1;
    s1.anchorLine = 5;
    collection.addShape(s1);
    
    DocumentShape s2;
    s2.anchorLine = 5;
    collection.addShape(s2);
    
    DocumentShape s3;
    s3.anchorLine = 10;
    collection.addShape(s3);

    SECTION("returns shapes at specified line") {
        auto atLine5 = collection.shapesAtLine(5);
        REQUIRE(atLine5.size() == 2);
    }

    SECTION("returns empty for line with no shapes") {
        auto atLine0 = collection.shapesAtLine(0);
        REQUIRE(atLine0.empty());
    }

    SECTION("const version works") {
        const ShapeCollection& constCollection = collection;
        auto atLine5 = constCollection.shapesAtLine(5);
        REQUIRE(atLine5.size() == 2);
    }
}

TEST_CASE("ShapeCollection shiftAnchorsFrom", "[drawing]") {
    ShapeCollection collection;
    
    DocumentShape s1;
    s1.anchorLine = 5;
    collection.addShape(s1);
    
    DocumentShape s2;
    s2.anchorLine = 10;
    collection.addShape(s2);

    SECTION("positive shift moves anchors down") {
        collection.shiftAnchorsFrom(5, 3);
        
        auto at8 = collection.shapesAtLine(8);
        REQUIRE(at8.size() == 1);
        
        auto at13 = collection.shapesAtLine(13);
        REQUIRE(at13.size() == 1);
    }

    SECTION("negative shift moves anchors up") {
        collection.shiftAnchorsFrom(0, -2);
        
        auto at3 = collection.shapesAtLine(3);
        REQUIRE(at3.size() == 1);
    }

    SECTION("shift only affects shapes at or after line") {
        collection.shiftAnchorsFrom(8, 5);
        
        auto at5 = collection.shapesAtLine(5);
        REQUIRE(at5.size() == 1);
        
        auto at15 = collection.shapesAtLine(15);
        REQUIRE(at15.size() == 1);
    }
}

TEST_CASE("ShapeCollection clear", "[drawing]") {
    ShapeCollection collection;
    DocumentShape s;
    collection.addShape(s);
    collection.addShape(s);
    REQUIRE(collection.count() == 2);

    SECTION("clear removes all shapes") {
        collection.clear();
        REQUIRE(collection.isEmpty());
        REQUIRE(collection.count() == 0);
    }

    SECTION("ID counter resets after clear") {
        collection.clear();
        std::size_t newId = collection.addShape(s);
        REQUIRE(newId == 1);
    }
}

TEST_CASE("ShapeCollection shapeAtPoint", "[drawing]") {
    ShapeCollection collection;
    
    DocumentShape s1;
    s1.position = {0.0f, 0.0f};
    s1.width = 100.0f;
    s1.height = 100.0f;
    collection.addShape(s1);
    
    DocumentShape s2;
    s2.position = {50.0f, 50.0f};
    s2.width = 100.0f;
    s2.height = 100.0f;
    collection.addShape(s2);

    SECTION("finds topmost shape at point") {
        // Point in overlapping region - should return s2 (added later)
        auto* found = collection.shapeAtPoint(75.0f, 75.0f);
        REQUIRE(found != nullptr);
        REQUIRE(found->position.x == 50.0f);
    }

    SECTION("finds shape at non-overlapping point") {
        auto* found = collection.shapeAtPoint(25.0f, 25.0f);
        REQUIRE(found != nullptr);
        REQUIRE(found->position.x == 0.0f);
    }

    SECTION("returns nullptr for point outside all shapes") {
        auto* found = collection.shapeAtPoint(200.0f, 200.0f);
        REQUIRE(found == nullptr);
    }
}

TEST_CASE("Factory functions", "[drawing]") {
    SECTION("createLine") {
        auto line = createLine({0.0f, 0.0f}, {100.0f, 50.0f}, 2.0f);
        REQUIRE(line.type == ShapeType::Line);
        REQUIRE(line.lineStart.x == 0.0f);
        REQUIRE(line.lineEnd.x == 100.0f);
        REQUIRE(line.stroke.width == 2.0f);
        REQUIRE(line.fill.style == FillStyle::None);
    }

    SECTION("createArrow") {
        auto arrow = createArrow({0.0f, 0.0f}, {100.0f, 0.0f});
        REQUIRE(arrow.type == ShapeType::Arrow);
        REQUIRE(arrow.stroke.endArrow == ArrowHead::Triangle);
    }

    SECTION("createRectangle") {
        auto rect = createRectangle(10.0f, 20.0f, 100.0f, 50.0f);
        REQUIRE(rect.type == ShapeType::Rectangle);
        REQUIRE(rect.position.x == 10.0f);
        REQUIRE(rect.position.y == 20.0f);
        REQUIRE(rect.width == 100.0f);
        REQUIRE(rect.height == 50.0f);
    }

    SECTION("createEllipse") {
        auto ellipse = createEllipse(0.0f, 0.0f, 80.0f, 40.0f);
        REQUIRE(ellipse.type == ShapeType::Ellipse);
        REQUIRE(ellipse.width == 80.0f);
        REQUIRE(ellipse.height == 40.0f);
    }

    SECTION("createRoundedRect") {
        auto rounded = createRoundedRect(0.0f, 0.0f, 100.0f, 50.0f, 12.0f);
        REQUIRE(rounded.type == ShapeType::RoundedRect);
        REQUIRE(rounded.cornerRadius == 12.0f);
    }
}

TEST_CASE("Stroke and fill enums", "[drawing]") {
    SECTION("StrokeStyle values") {
        REQUIRE(static_cast<int>(StrokeStyle::None) == 0);
        REQUIRE(static_cast<int>(StrokeStyle::Solid) == 1);
        REQUIRE(static_cast<int>(StrokeStyle::Dashed) == 2);
        REQUIRE(static_cast<int>(StrokeStyle::Dotted) == 3);
    }

    SECTION("FillStyle values") {
        REQUIRE(static_cast<int>(FillStyle::None) == 0);
        REQUIRE(static_cast<int>(FillStyle::Solid) == 1);
    }

    SECTION("ArrowHead values") {
        REQUIRE(static_cast<int>(ArrowHead::None) == 0);
        REQUIRE(static_cast<int>(ArrowHead::Triangle) == 1);
    }
}
