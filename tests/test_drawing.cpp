#include "../src/editor/drawing.h"
#include "catch2/catch.hpp"

TEST_CASE("ShapeType names", "[drawing]") {
    REQUIRE(std::string(shapeTypeName(ShapeType::Line)) == "Line");
    REQUIRE(std::string(shapeTypeName(ShapeType::Rectangle)) == "Rectangle");
    REQUIRE(std::string(shapeTypeName(ShapeType::Ellipse)) == "Ellipse");
    REQUIRE(std::string(shapeTypeName(ShapeType::Arrow)) == "Arrow");
    REQUIRE(std::string(shapeTypeName(ShapeType::RoundedRect)) == "Rounded Rectangle");
    REQUIRE(std::string(shapeTypeName(ShapeType::Triangle)) == "Triangle");
    REQUIRE(std::string(shapeTypeName(ShapeType::FreeformLine)) == "Freeform Line");
}

TEST_CASE("LineStyle names", "[drawing]") {
    REQUIRE(std::string(lineStyleName(LineStyle::Solid)) == "Solid");
    REQUIRE(std::string(lineStyleName(LineStyle::Dashed)) == "Dashed");
    REQUIRE(std::string(lineStyleName(LineStyle::Dotted)) == "Dotted");
    REQUIRE(std::string(lineStyleName(LineStyle::DashDot)) == "Dash-Dot");
}

TEST_CASE("DrawingLayoutMode names", "[drawing]") {
    REQUIRE(std::string(drawingLayoutModeName(DrawingLayoutMode::Inline)) == "Inline with Text");
    REQUIRE(std::string(drawingLayoutModeName(DrawingLayoutMode::Float)) == "Float");
    REQUIRE(std::string(drawingLayoutModeName(DrawingLayoutMode::Behind)) == "Behind Text");
    REQUIRE(std::string(drawingLayoutModeName(DrawingLayoutMode::InFront)) == "In Front of Text");
}

TEST_CASE("DrawingColor", "[drawing]") {
    SECTION("default construction") {
        DrawingColor color;
        REQUIRE(color.r == 0);
        REQUIRE(color.g == 0);
        REQUIRE(color.b == 0);
        REQUIRE(color.a == 255);
    }
    
    SECTION("equality") {
        DrawingColor a = {100, 150, 200, 255};
        DrawingColor b = {100, 150, 200, 255};
        DrawingColor c = {100, 150, 201, 255};
        REQUIRE(a == b);
        REQUIRE(a != c);
    }
    
    SECTION("transparency check") {
        DrawingColor opaque = {0, 0, 0, 255};
        DrawingColor transparent = {0, 0, 0, 0};
        REQUIRE_FALSE(opaque.isTransparent());
        REQUIRE(transparent.isTransparent());
    }
    
    SECTION("predefined colors") {
        REQUIRE(DrawingColors::Black.r == 0);
        REQUIRE(DrawingColors::White.r == 255);
        REQUIRE(DrawingColors::Red.r == 255);
        REQUIRE(DrawingColors::Green.g == 255);
        REQUIRE(DrawingColors::Blue.b == 255);
        REQUIRE(DrawingColors::Transparent.a == 0);
    }
}

TEST_CASE("DrawingPoint", "[drawing]") {
    SECTION("default construction") {
        DrawingPoint pt;
        REQUIRE(pt.x == 0.0f);
        REQUIRE(pt.y == 0.0f);
    }
    
    SECTION("equality") {
        DrawingPoint a = {10.0f, 20.0f};
        DrawingPoint b = {10.0f, 20.0f};
        DrawingPoint c = {10.0f, 21.0f};
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
    }
}

TEST_CASE("DocumentDrawing struct", "[drawing]") {
    SECTION("default construction") {
        DocumentDrawing drw;
        REQUIRE(drw.id == 0);
        REQUIRE(drw.shapeType == ShapeType::Line);
        REQUIRE(drw.anchorLine == 0);
        REQUIRE(drw.anchorColumn == 0);
        REQUIRE(drw.width == Approx(100.0f));
        REQUIRE(drw.height == Approx(50.0f));
        REQUIRE(drw.layoutMode == DrawingLayoutMode::Inline);
        REQUIRE(drw.strokeWidth == Approx(1.0f));
        REQUIRE(drw.lineStyle == LineStyle::Solid);
        REQUIRE_FALSE(drw.filled);
    }
    
    SECTION("set size") {
        DocumentDrawing drw;
        drw.setSize(200.0f, 100.0f);
        REQUIRE(drw.getWidth() == Approx(200.0f));
        REQUIRE(drw.getHeight() == Approx(100.0f));
    }
    
    SECTION("get bounds") {
        DocumentDrawing drw;
        drw.x = 10.0f;
        drw.y = 20.0f;
        drw.width = 100.0f;
        drw.height = 50.0f;
        
        auto bounds = drw.getBounds();
        REQUIRE(bounds.x == Approx(10.0f));
        REQUIRE(bounds.y == Approx(20.0f));
        REQUIRE(bounds.width == Approx(100.0f));
        REQUIRE(bounds.height == Approx(50.0f));
    }
    
    SECTION("contains point") {
        DocumentDrawing drw;
        drw.x = 10.0f;
        drw.y = 20.0f;
        drw.width = 100.0f;
        drw.height = 50.0f;
        
        // Inside
        REQUIRE(drw.containsPoint(50.0f, 40.0f));
        // On edge
        REQUIRE(drw.containsPoint(10.0f, 20.0f));
        REQUIRE(drw.containsPoint(110.0f, 70.0f));
        // Outside
        REQUIRE_FALSE(drw.containsPoint(5.0f, 40.0f));
        REQUIRE_FALSE(drw.containsPoint(50.0f, 80.0f));
    }
    
    SECTION("freeform line points") {
        DocumentDrawing drw;
        drw.shapeType = ShapeType::FreeformLine;
        drw.points.push_back({0.0f, 0.0f});
        drw.points.push_back({10.0f, 10.0f});
        drw.points.push_back({20.0f, 5.0f});
        REQUIRE(drw.points.size() == 3);
    }
    
    SECTION("arrow properties") {
        DocumentDrawing drw;
        drw.shapeType = ShapeType::Arrow;
        drw.startArrow = ArrowStyle::None;
        drw.endArrow = ArrowStyle::Standard;
        REQUIRE(drw.startArrow == ArrowStyle::None);
        REQUIRE(drw.endArrow == ArrowStyle::Standard);
    }
}

TEST_CASE("DrawingCollection", "[drawing]") {
    SECTION("add and get drawing") {
        DrawingCollection coll;
        REQUIRE(coll.isEmpty());
        REQUIRE(coll.count() == 0);
        
        DocumentDrawing drw;
        drw.shapeType = ShapeType::Rectangle;
        drw.anchorLine = 5;
        
        std::size_t id = coll.addDrawing(drw);
        REQUIRE(id > 0);
        REQUIRE_FALSE(coll.isEmpty());
        REQUIRE(coll.count() == 1);
        
        auto* retrieved = coll.getDrawing(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->shapeType == ShapeType::Rectangle);
        REQUIRE(retrieved->anchorLine == 5);
    }
    
    SECTION("remove drawing") {
        DrawingCollection coll;
        DocumentDrawing drw;
        std::size_t id = coll.addDrawing(drw);
        REQUIRE(coll.count() == 1);
        
        REQUIRE(coll.removeDrawing(id));
        REQUIRE(coll.count() == 0);
        REQUIRE(coll.getDrawing(id) == nullptr);
    }
    
    SECTION("remove nonexistent drawing returns false") {
        DrawingCollection coll;
        REQUIRE_FALSE(coll.removeDrawing(999));
    }
    
    SECTION("drawings at line") {
        DrawingCollection coll;
        
        DocumentDrawing drw1;
        drw1.shapeType = ShapeType::Line;
        drw1.anchorLine = 5;
        coll.addDrawing(drw1);
        
        DocumentDrawing drw2;
        drw2.shapeType = ShapeType::Ellipse;
        drw2.anchorLine = 5;
        coll.addDrawing(drw2);
        
        DocumentDrawing drw3;
        drw3.shapeType = ShapeType::Rectangle;
        drw3.anchorLine = 10;
        coll.addDrawing(drw3);
        
        auto atLine5 = coll.drawingsAtLine(5);
        REQUIRE(atLine5.size() == 2);
        
        auto atLine10 = coll.drawingsAtLine(10);
        REQUIRE(atLine10.size() == 1);
        REQUIRE(atLine10[0]->shapeType == ShapeType::Rectangle);
        
        auto atLine7 = coll.drawingsAtLine(7);
        REQUIRE(atLine7.empty());
    }
    
    SECTION("drawings in range") {
        DrawingCollection coll;
        
        DocumentDrawing drw1;
        drw1.anchorLine = 5;
        coll.addDrawing(drw1);
        
        DocumentDrawing drw2;
        drw2.anchorLine = 10;
        coll.addDrawing(drw2);
        
        DocumentDrawing drw3;
        drw3.anchorLine = 15;
        coll.addDrawing(drw3);
        
        auto inRange = coll.drawingsInRange(4, 12);
        REQUIRE(inRange.size() == 2);  // Lines 5 and 10
    }
    
    SECTION("shift anchors from positive delta") {
        DrawingCollection coll;
        
        DocumentDrawing drw1;
        drw1.anchorLine = 5;
        std::size_t id1 = coll.addDrawing(drw1);
        
        DocumentDrawing drw2;
        drw2.anchorLine = 10;
        std::size_t id2 = coll.addDrawing(drw2);
        
        DocumentDrawing drw3;
        drw3.anchorLine = 15;
        std::size_t id3 = coll.addDrawing(drw3);
        
        // Insert 3 lines at line 8
        coll.shiftAnchorsFrom(8, 3);
        
        REQUIRE(coll.getDrawing(id1)->anchorLine == 5);   // Before line 8, unchanged
        REQUIRE(coll.getDrawing(id2)->anchorLine == 13);  // 10 + 3
        REQUIRE(coll.getDrawing(id3)->anchorLine == 18);  // 15 + 3
    }
    
    SECTION("shift anchors negative delta") {
        DrawingCollection coll;
        
        DocumentDrawing drw;
        drw.anchorLine = 10;
        std::size_t id = coll.addDrawing(drw);
        
        // Delete 3 lines starting at line 8
        coll.shiftAnchorsFrom(8, -3);
        
        REQUIRE(coll.getDrawing(id)->anchorLine == 7);  // 10 - 3
    }
    
    SECTION("clear collection") {
        DrawingCollection coll;
        
        DocumentDrawing drw;
        coll.addDrawing(drw);
        coll.addDrawing(drw);
        coll.addDrawing(drw);
        REQUIRE(coll.count() == 3);
        
        coll.clear();
        REQUIRE(coll.isEmpty());
        REQUIRE(coll.count() == 0);
    }
    
    SECTION("drawing IDs are unique and auto-incrementing") {
        DrawingCollection coll;
        
        DocumentDrawing drw;
        std::size_t id1 = coll.addDrawing(drw);
        std::size_t id2 = coll.addDrawing(drw);
        std::size_t id3 = coll.addDrawing(drw);
        
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
        REQUIRE(id1 < id2);
        REQUIRE(id2 < id3);
    }
}
