#include "../src/editor/image.h"
#include "catch2/catch.hpp"

TEST_CASE("DocumentImage struct", "[image]") {
    SECTION("default construction") {
        DocumentImage img;
        REQUIRE(img.id == 0);
        REQUIRE(img.filename.empty());
        REQUIRE(img.base64Data.empty());
        REQUIRE(img.isEmbedded == true);
        REQUIRE(img.anchorLine == 0);
        REQUIRE(img.anchorColumn == 0);
        REQUIRE(img.layoutMode == ImageLayoutMode::Inline);
        REQUIRE(img.alignment == ImageAlignment::Left);
    }
    
    SECTION("aspect ratio calculation") {
        DocumentImage img;
        img.originalWidth = 400.0f;
        img.originalHeight = 200.0f;
        REQUIRE(img.aspectRatio() == Approx(2.0f));
    }
    
    SECTION("set display width maintains aspect ratio") {
        DocumentImage img;
        img.originalWidth = 400.0f;
        img.originalHeight = 200.0f;
        img.setDisplayWidth(200.0f);
        REQUIRE(img.displayWidth == Approx(200.0f));
        REQUIRE(img.displayHeight == Approx(100.0f));
    }
    
    SECTION("set display height maintains aspect ratio") {
        DocumentImage img;
        img.originalWidth = 400.0f;
        img.originalHeight = 200.0f;
        img.setDisplayHeight(100.0f);
        REQUIRE(img.displayHeight == Approx(100.0f));
        REQUIRE(img.displayWidth == Approx(200.0f));
    }
    
    SECTION("reset size to original") {
        DocumentImage img;
        img.originalWidth = 400.0f;
        img.originalHeight = 200.0f;
        img.displayWidth = 100.0f;
        img.displayHeight = 50.0f;
        img.resetSize();
        REQUIRE(img.displayWidth == Approx(400.0f));
        REQUIRE(img.displayHeight == Approx(200.0f));
    }
    
    SECTION("has embedded data") {
        DocumentImage img;
        REQUIRE_FALSE(img.hasEmbeddedData());
        img.base64Data = "SGVsbG8gV29ybGQ=";  // Base64 for "Hello World"
        REQUIRE(img.hasEmbeddedData());
    }
    
    SECTION("has external source") {
        DocumentImage img;
        img.isEmbedded = false;
        REQUIRE_FALSE(img.hasExternalSource());
        img.filename = "test.png";
        REQUIRE(img.hasExternalSource());
    }
    
    SECTION("bounds calculation") {
        DocumentImage img;
        img.displayWidth = 100.0f;
        img.displayHeight = 80.0f;
        img.marginLeft = 5.0f;
        img.marginRight = 5.0f;
        img.marginTop = 10.0f;
        img.marginBottom = 10.0f;
        img.borderWidth = 2.0f;
        
        auto bounds = img.getBounds(50.0f, 100.0f);
        // x = anchorX + offsetX - marginLeft - borderWidth = 50 + 0 - 5 - 2 = 43
        REQUIRE(bounds.x == Approx(43.0f));
        // y = anchorY + offsetY - marginTop - borderWidth = 100 + 0 - 10 - 2 = 88
        REQUIRE(bounds.y == Approx(88.0f));
        // width = displayWidth + marginLeft + marginRight + borderWidth*2 = 100 + 5 + 5 + 4 = 114
        REQUIRE(bounds.width == Approx(114.0f));
        // height = displayHeight + marginTop + marginBottom + borderWidth*2 = 80 + 10 + 10 + 4 = 104
        REQUIRE(bounds.height == Approx(104.0f));
    }
}

TEST_CASE("ImageCollection", "[image]") {
    SECTION("add and get image") {
        ImageCollection coll;
        REQUIRE(coll.isEmpty());
        REQUIRE(coll.count() == 0);
        
        DocumentImage img;
        img.filename = "test.png";
        img.anchorLine = 5;
        
        std::size_t id = coll.addImage(img);
        REQUIRE(id > 0);
        REQUIRE_FALSE(coll.isEmpty());
        REQUIRE(coll.count() == 1);
        
        auto* retrieved = coll.getImage(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->filename == "test.png");
        REQUIRE(retrieved->anchorLine == 5);
    }
    
    SECTION("remove image") {
        ImageCollection coll;
        DocumentImage img;
        img.filename = "to_remove.png";
        std::size_t id = coll.addImage(img);
        REQUIRE(coll.count() == 1);
        
        REQUIRE(coll.removeImage(id));
        REQUIRE(coll.count() == 0);
        REQUIRE(coll.getImage(id) == nullptr);
    }
    
    SECTION("remove nonexistent image returns false") {
        ImageCollection coll;
        REQUIRE_FALSE(coll.removeImage(999));
    }
    
    SECTION("images at line") {
        ImageCollection coll;
        
        DocumentImage img1;
        img1.filename = "img1.png";
        img1.anchorLine = 5;
        coll.addImage(img1);
        
        DocumentImage img2;
        img2.filename = "img2.png";
        img2.anchorLine = 5;
        coll.addImage(img2);
        
        DocumentImage img3;
        img3.filename = "img3.png";
        img3.anchorLine = 10;
        coll.addImage(img3);
        
        auto atLine5 = coll.imagesAtLine(5);
        REQUIRE(atLine5.size() == 2);
        
        auto atLine10 = coll.imagesAtLine(10);
        REQUIRE(atLine10.size() == 1);
        REQUIRE(atLine10[0]->filename == "img3.png");
        
        auto atLine7 = coll.imagesAtLine(7);
        REQUIRE(atLine7.empty());
    }
    
    SECTION("images in range") {
        ImageCollection coll;
        
        DocumentImage img1;
        img1.anchorLine = 5;
        coll.addImage(img1);
        
        DocumentImage img2;
        img2.anchorLine = 10;
        coll.addImage(img2);
        
        DocumentImage img3;
        img3.anchorLine = 15;
        coll.addImage(img3);
        
        auto inRange = coll.imagesInRange(4, 12);
        REQUIRE(inRange.size() == 2);  // Lines 5 and 10
    }
    
    SECTION("shift anchors from") {
        ImageCollection coll;
        
        DocumentImage img1;
        img1.anchorLine = 5;
        std::size_t id1 = coll.addImage(img1);
        
        DocumentImage img2;
        img2.anchorLine = 10;
        std::size_t id2 = coll.addImage(img2);
        
        DocumentImage img3;
        img3.anchorLine = 15;
        std::size_t id3 = coll.addImage(img3);
        
        // Insert 3 lines at line 8
        coll.shiftAnchorsFrom(8, 3);
        
        REQUIRE(coll.getImage(id1)->anchorLine == 5);   // Before line 8, unchanged
        REQUIRE(coll.getImage(id2)->anchorLine == 13);  // 10 + 3
        REQUIRE(coll.getImage(id3)->anchorLine == 18);  // 15 + 3
    }
    
    SECTION("shift anchors negative delta") {
        ImageCollection coll;
        
        DocumentImage img;
        img.anchorLine = 10;
        std::size_t id = coll.addImage(img);
        
        // Delete 3 lines starting at line 8
        coll.shiftAnchorsFrom(8, -3);
        
        REQUIRE(coll.getImage(id)->anchorLine == 7);  // 10 - 3
    }
    
    SECTION("clear collection") {
        ImageCollection coll;
        
        DocumentImage img;
        coll.addImage(img);
        coll.addImage(img);
        coll.addImage(img);
        REQUIRE(coll.count() == 3);
        
        coll.clear();
        REQUIRE(coll.isEmpty());
        REQUIRE(coll.count() == 0);
    }
    
    SECTION("image IDs are unique and auto-incrementing") {
        ImageCollection coll;
        
        DocumentImage img;
        std::size_t id1 = coll.addImage(img);
        std::size_t id2 = coll.addImage(img);
        std::size_t id3 = coll.addImage(img);
        
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
        REQUIRE(id1 < id2);
        REQUIRE(id2 < id3);
    }
}

TEST_CASE("ImageLayoutMode", "[image]") {
    SECTION("layout mode names") {
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Inline)) == "Inline with Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapSquare)) == "Square Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapTight)) == "Tight Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::BreakText)) == "Break Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Behind)) == "Behind Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::InFront)) == "In Front of Text");
    }
}
