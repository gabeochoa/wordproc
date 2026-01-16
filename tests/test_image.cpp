#include "catch2/catch.hpp"
#include "../src/editor/image.h"

TEST_CASE("DocumentImage creation and properties", "[image]") {
    SECTION("Default values") {
        DocumentImage img;
        REQUIRE(img.filename.empty());
        REQUIRE(img.isEmbedded == true);
        REQUIRE(img.anchorLine == 0);
        REQUIRE(img.originalWidth == 0.0f);
        REQUIRE(img.layoutMode == ImageLayoutMode::Inline);
        REQUIRE(img.alignment == ImageAlignment::Left);
    }
    
    SECTION("Set image properties") {
        DocumentImage img;
        img.filename = "test.png";
        img.originalWidth = 800.0f;
        img.originalHeight = 600.0f;
        img.displayWidth = 400.0f;
        img.displayHeight = 300.0f;
        img.layoutMode = ImageLayoutMode::WrapSquare;
        img.alignment = ImageAlignment::Center;
        
        REQUIRE(img.filename == "test.png");
        REQUIRE(img.originalWidth == 800.0f);
        REQUIRE(img.layoutMode == ImageLayoutMode::WrapSquare);
        REQUIRE(img.alignment == ImageAlignment::Center);
    }
    
    SECTION("Aspect ratio calculation") {
        DocumentImage img;
        img.originalWidth = 1600.0f;
        img.originalHeight = 900.0f;
        
        REQUIRE(img.aspectRatio() == Approx(1600.0f / 900.0f));
    }
    
    SECTION("Set display width maintains aspect ratio") {
        DocumentImage img;
        img.originalWidth = 1600.0f;
        img.originalHeight = 900.0f;
        
        img.setDisplayWidth(800.0f);
        REQUIRE(img.displayWidth == 800.0f);
        REQUIRE(img.displayHeight == Approx(450.0f));
    }
    
    SECTION("Set display height maintains aspect ratio") {
        DocumentImage img;
        img.originalWidth = 1600.0f;
        img.originalHeight = 900.0f;
        
        img.setDisplayHeight(450.0f);
        REQUIRE(img.displayHeight == 450.0f);
        REQUIRE(img.displayWidth == Approx(800.0f));
    }
    
    SECTION("Reset size restores original dimensions") {
        DocumentImage img;
        img.originalWidth = 1000.0f;
        img.originalHeight = 800.0f;
        img.displayWidth = 500.0f;
        img.displayHeight = 400.0f;
        
        img.resetSize();
        REQUIRE(img.displayWidth == 1000.0f);
        REQUIRE(img.displayHeight == 800.0f);
    }
    
    SECTION("Has embedded data check") {
        DocumentImage img;
        REQUIRE_FALSE(img.hasEmbeddedData());
        
        img.base64Data = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAA";
        REQUIRE(img.hasEmbeddedData());
    }
    
    SECTION("Has external source check") {
        DocumentImage img;
        REQUIRE_FALSE(img.hasExternalSource());
        
        img.isEmbedded = false;
        img.filename = "/path/to/image.png";
        REQUIRE(img.hasExternalSource());
    }
}

TEST_CASE("DocumentImage bounds calculation", "[image]") {
    SECTION("Simple bounds without offset") {
        DocumentImage img;
        img.displayWidth = 200.0f;
        img.displayHeight = 150.0f;
        img.marginTop = 4.0f;
        img.marginBottom = 4.0f;
        img.marginLeft = 4.0f;
        img.marginRight = 4.0f;
        img.borderWidth = 0.0f;
        
        DocumentImage::Bounds bounds = img.getBounds(100.0f, 50.0f);
        REQUIRE(bounds.x == Approx(100.0f - 4.0f));  // anchorX - marginLeft
        REQUIRE(bounds.y == Approx(50.0f - 4.0f));   // anchorY - marginTop
        REQUIRE(bounds.width == Approx(200.0f + 8.0f));  // displayWidth + margins
        REQUIRE(bounds.height == Approx(150.0f + 8.0f)); // displayHeight + margins
    }
    
    SECTION("Bounds with offset") {
        DocumentImage img;
        img.displayWidth = 200.0f;
        img.displayHeight = 150.0f;
        img.offsetX = 20.0f;
        img.offsetY = 10.0f;
        img.marginTop = 0.0f;
        img.marginBottom = 0.0f;
        img.marginLeft = 0.0f;
        img.marginRight = 0.0f;
        
        DocumentImage::Bounds bounds = img.getBounds(100.0f, 50.0f);
        REQUIRE(bounds.x == Approx(120.0f));  // anchorX + offsetX
        REQUIRE(bounds.y == Approx(60.0f));   // anchorY + offsetY
    }
    
    SECTION("Bounds with border") {
        DocumentImage img;
        img.displayWidth = 100.0f;
        img.displayHeight = 100.0f;
        img.marginTop = 0.0f;
        img.marginBottom = 0.0f;
        img.marginLeft = 0.0f;
        img.marginRight = 0.0f;
        img.borderWidth = 2.0f;
        
        DocumentImage::Bounds bounds = img.getBounds(0.0f, 0.0f);
        REQUIRE(bounds.width == Approx(104.0f));  // displayWidth + 2*borderWidth
        REQUIRE(bounds.height == Approx(104.0f)); // displayHeight + 2*borderWidth
    }
}

TEST_CASE("ImageCollection management", "[image]") {
    ImageCollection collection;
    
    SECTION("Initially empty") {
        REQUIRE(collection.isEmpty());
        REQUIRE(collection.count() == 0);
    }
    
    SECTION("Add image returns unique ID") {
        DocumentImage img1;
        img1.filename = "img1.png";
        std::size_t id1 = collection.addImage(img1);
        
        DocumentImage img2;
        img2.filename = "img2.png";
        std::size_t id2 = collection.addImage(img2);
        
        REQUIRE(id1 != id2);
        REQUIRE(collection.count() == 2);
    }
    
    SECTION("Get image by ID") {
        DocumentImage img;
        img.filename = "test.png";
        std::size_t id = collection.addImage(img);
        
        DocumentImage* retrieved = collection.getImage(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->filename == "test.png");
    }
    
    SECTION("Get non-existent image returns nullptr") {
        REQUIRE(collection.getImage(999) == nullptr);
    }
    
    SECTION("Remove image by ID") {
        DocumentImage img;
        std::size_t id = collection.addImage(img);
        REQUIRE(collection.count() == 1);
        
        bool removed = collection.removeImage(id);
        REQUIRE(removed);
        REQUIRE(collection.count() == 0);
    }
    
    SECTION("Remove non-existent image returns false") {
        REQUIRE_FALSE(collection.removeImage(999));
    }
    
    SECTION("Clear removes all images") {
        collection.addImage(DocumentImage{});
        collection.addImage(DocumentImage{});
        collection.addImage(DocumentImage{});
        REQUIRE(collection.count() == 3);
        
        collection.clear();
        REQUIRE(collection.isEmpty());
    }
}

TEST_CASE("ImageCollection line queries", "[image]") {
    ImageCollection collection;
    
    DocumentImage img1;
    img1.anchorLine = 5;
    collection.addImage(img1);
    
    DocumentImage img2;
    img2.anchorLine = 5;
    collection.addImage(img2);
    
    DocumentImage img3;
    img3.anchorLine = 10;
    collection.addImage(img3);
    
    SECTION("Get images at specific line") {
        auto images = collection.imagesAtLine(5);
        REQUIRE(images.size() == 2);
        
        images = collection.imagesAtLine(10);
        REQUIRE(images.size() == 1);
        
        images = collection.imagesAtLine(15);
        REQUIRE(images.empty());
    }
    
    SECTION("Get images in range") {
        auto images = collection.imagesInRange(0, 6);
        REQUIRE(images.size() == 2);
        
        images = collection.imagesInRange(8, 15);
        REQUIRE(images.size() == 1);
        
        images = collection.imagesInRange(0, 15);
        REQUIRE(images.size() == 3);
    }
    
    SECTION("Shift anchors from line") {
        collection.shiftAnchorsFrom(7, 3);
        
        auto images = collection.imagesAtLine(5);
        REQUIRE(images.size() == 2);  // Still at line 5
        
        images = collection.imagesAtLine(13);  // 10 + 3
        REQUIRE(images.size() == 1);
    }
    
    SECTION("Shift anchors negative") {
        collection.shiftAnchorsFrom(7, -2);
        
        auto images = collection.imagesAtLine(5);
        REQUIRE(images.size() == 2);  // Still at line 5
        
        images = collection.imagesAtLine(8);  // 10 - 2
        REQUIRE(images.size() == 1);
    }
}

TEST_CASE("ImageLayoutMode names", "[image]") {
    SECTION("Layout mode names") {
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Inline)) == "Inline with Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapSquare)) == "Square Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapTight)) == "Tight Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::BreakText)) == "Break Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Behind)) == "Behind Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::InFront)) == "In Front of Text");
    }
}

TEST_CASE("Image styling", "[image]") {
    DocumentImage img;
    
    SECTION("Set border properties") {
        img.borderWidth = 3.0f;
        img.borderR = 255;
        img.borderG = 0;
        img.borderB = 0;
        img.borderA = 255;
        
        REQUIRE(img.borderWidth == 3.0f);
        REQUIRE(img.borderR == 255);
    }
    
    SECTION("Set margins") {
        img.marginTop = 10.0f;
        img.marginBottom = 10.0f;
        img.marginLeft = 20.0f;
        img.marginRight = 20.0f;
        
        REQUIRE(img.marginTop == 10.0f);
        REQUIRE(img.marginLeft == 20.0f);
    }
    
    SECTION("Set alt text") {
        img.altText = "A beautiful sunset over the mountains";
        REQUIRE(img.altText == "A beautiful sunset over the mountains");
    }
}
