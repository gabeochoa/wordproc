#include "../src/editor/image.h"
#include "catch2/catch.hpp"

TEST_CASE("DocumentImage initialization", "[image]") {
    DocumentImage img;
    
    SECTION("default values") {
        REQUIRE(img.filename.empty());
        REQUIRE(img.base64Data.empty());
        REQUIRE(img.isEmbedded == true);
        REQUIRE(img.anchorLine == 0);
        REQUIRE(img.anchorColumn == 0);
        REQUIRE(img.originalWidth == 0.0f);
        REQUIRE(img.originalHeight == 0.0f);
        REQUIRE(img.layoutMode == ImageLayoutMode::Inline);
        REQUIRE(img.alignment == ImageAlignment::Left);
    }

    SECTION("helper methods for empty image") {
        REQUIRE_FALSE(img.hasEmbeddedData());
        REQUIRE_FALSE(img.hasExternalSource());
        REQUIRE(img.aspectRatio() == 1.0f);  // Default for 0 height
    }
}

TEST_CASE("DocumentImage embedded vs external", "[image]") {
    SECTION("embedded image") {
        DocumentImage img;
        img.isEmbedded = true;
        img.base64Data = "SGVsbG8gV29ybGQ=";
        REQUIRE(img.hasEmbeddedData());
        REQUIRE_FALSE(img.hasExternalSource());
    }

    SECTION("external image") {
        DocumentImage img;
        img.isEmbedded = false;
        img.filename = "images/photo.png";
        REQUIRE_FALSE(img.hasEmbeddedData());
        REQUIRE(img.hasExternalSource());
    }
}

TEST_CASE("DocumentImage aspect ratio", "[image]") {
    DocumentImage img;
    
    SECTION("landscape image") {
        img.originalWidth = 800.0f;
        img.originalHeight = 600.0f;
        REQUIRE(img.aspectRatio() == Catch::Approx(800.0f / 600.0f));
    }

    SECTION("portrait image") {
        img.originalWidth = 600.0f;
        img.originalHeight = 800.0f;
        REQUIRE(img.aspectRatio() == Catch::Approx(600.0f / 800.0f));
    }

    SECTION("square image") {
        img.originalWidth = 500.0f;
        img.originalHeight = 500.0f;
        REQUIRE(img.aspectRatio() == Catch::Approx(1.0f));
    }
}

TEST_CASE("DocumentImage size operations", "[image]") {
    DocumentImage img;
    img.originalWidth = 800.0f;
    img.originalHeight = 600.0f;

    SECTION("setDisplayWidth maintains aspect ratio") {
        img.setDisplayWidth(400.0f);
        REQUIRE(img.displayWidth == 400.0f);
        REQUIRE(img.displayHeight == Catch::Approx(300.0f));
    }

    SECTION("setDisplayHeight maintains aspect ratio") {
        img.setDisplayHeight(300.0f);
        REQUIRE(img.displayHeight == 300.0f);
        REQUIRE(img.displayWidth == Catch::Approx(400.0f));
    }

    SECTION("resetSize restores original dimensions") {
        img.setDisplayWidth(100.0f);
        img.resetSize();
        REQUIRE(img.displayWidth == 800.0f);
        REQUIRE(img.displayHeight == 600.0f);
    }
}

TEST_CASE("DocumentImage bounds calculation", "[image]") {
    DocumentImage img;
    img.displayWidth = 100.0f;
    img.displayHeight = 80.0f;
    img.marginTop = 5.0f;
    img.marginBottom = 5.0f;
    img.marginLeft = 10.0f;
    img.marginRight = 10.0f;
    img.borderWidth = 2.0f;

    SECTION("getBounds includes margins and border") {
        auto bounds = img.getBounds(50.0f, 100.0f);
        float expectedWidth = 100.0f + 10.0f + 10.0f + 4.0f;  // width + margins + borders
        float expectedHeight = 80.0f + 5.0f + 5.0f + 4.0f;    // height + margins + borders
        REQUIRE(bounds.width == Catch::Approx(expectedWidth));
        REQUIRE(bounds.height == Catch::Approx(expectedHeight));
    }
}

TEST_CASE("ImageLayoutMode names", "[image]") {
    SECTION("all modes have display names") {
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Inline)) == "Inline with Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapSquare)) == "Square Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::WrapTight)) == "Tight Wrap");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::BreakText)) == "Break Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::Behind)) == "Behind Text");
        REQUIRE(std::string(imageLayoutModeName(ImageLayoutMode::InFront)) == "In Front of Text");
    }
}

TEST_CASE("ImageCollection initialization", "[image]") {
    ImageCollection collection;
    
    SECTION("starts empty") {
        REQUIRE(collection.isEmpty());
        REQUIRE(collection.count() == 0);
        REQUIRE(collection.images().empty());
    }
}

TEST_CASE("ImageCollection add and get", "[image]") {
    ImageCollection collection;
    
    SECTION("addImage assigns unique IDs") {
        DocumentImage img1;
        img1.filename = "image1.png";
        std::size_t id1 = collection.addImage(img1);
        
        DocumentImage img2;
        img2.filename = "image2.png";
        std::size_t id2 = collection.addImage(img2);
        
        REQUIRE(id1 != id2);
        REQUIRE(collection.count() == 2);
    }

    SECTION("getImage retrieves by ID") {
        DocumentImage img;
        img.filename = "test.png";
        std::size_t id = collection.addImage(img);
        
        DocumentImage* retrieved = collection.getImage(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->filename == "test.png");
    }

    SECTION("getImage returns nullptr for invalid ID") {
        REQUIRE(collection.getImage(999) == nullptr);
    }

    SECTION("const getImage works") {
        DocumentImage img;
        img.filename = "test.png";
        std::size_t id = collection.addImage(img);
        
        const ImageCollection& constCollection = collection;
        const DocumentImage* retrieved = constCollection.getImage(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->filename == "test.png");
    }
}

TEST_CASE("ImageCollection remove", "[image]") {
    ImageCollection collection;
    DocumentImage img;
    img.filename = "test.png";
    std::size_t id = collection.addImage(img);

    SECTION("removeImage removes existing image") {
        REQUIRE(collection.count() == 1);
        bool removed = collection.removeImage(id);
        REQUIRE(removed);
        REQUIRE(collection.count() == 0);
        REQUIRE(collection.getImage(id) == nullptr);
    }

    SECTION("removeImage returns false for non-existent ID") {
        bool removed = collection.removeImage(999);
        REQUIRE_FALSE(removed);
        REQUIRE(collection.count() == 1);
    }
}

TEST_CASE("ImageCollection imagesAtLine", "[image]") {
    ImageCollection collection;
    
    DocumentImage img1;
    img1.anchorLine = 5;
    img1.filename = "img1.png";
    collection.addImage(img1);
    
    DocumentImage img2;
    img2.anchorLine = 5;
    img2.filename = "img2.png";
    collection.addImage(img2);
    
    DocumentImage img3;
    img3.anchorLine = 10;
    img3.filename = "img3.png";
    collection.addImage(img3);

    SECTION("returns images at specified line") {
        auto atLine5 = collection.imagesAtLine(5);
        REQUIRE(atLine5.size() == 2);
    }

    SECTION("returns empty for line with no images") {
        auto atLine0 = collection.imagesAtLine(0);
        REQUIRE(atLine0.empty());
    }

    SECTION("const version works") {
        const ImageCollection& constCollection = collection;
        auto atLine5 = constCollection.imagesAtLine(5);
        REQUIRE(atLine5.size() == 2);
    }
}

TEST_CASE("ImageCollection imagesInRange", "[image]") {
    ImageCollection collection;
    
    DocumentImage img1;
    img1.anchorLine = 5;
    img1.layoutMode = ImageLayoutMode::Inline;
    collection.addImage(img1);
    
    DocumentImage img2;
    img2.anchorLine = 10;
    img2.layoutMode = ImageLayoutMode::Inline;
    collection.addImage(img2);
    
    DocumentImage img3;
    img3.anchorLine = 15;
    img3.layoutMode = ImageLayoutMode::Inline;
    collection.addImage(img3);

    SECTION("returns images in range") {
        auto inRange = collection.imagesInRange(4, 11);
        REQUIRE(inRange.size() == 2);  // img1 and img2
    }

    SECTION("returns empty for range with no images") {
        auto inRange = collection.imagesInRange(100, 200);
        REQUIRE(inRange.empty());
    }
}

TEST_CASE("ImageCollection shiftAnchorsFrom", "[image]") {
    ImageCollection collection;
    
    DocumentImage img1;
    img1.anchorLine = 5;
    collection.addImage(img1);
    
    DocumentImage img2;
    img2.anchorLine = 10;
    collection.addImage(img2);

    SECTION("positive shift moves anchors down") {
        collection.shiftAnchorsFrom(5, 3);
        
        // First image shifted (was at line 5)
        auto at8 = collection.imagesAtLine(8);
        REQUIRE(at8.size() == 1);
        
        // Second image shifted (was at line 10)
        auto at13 = collection.imagesAtLine(13);
        REQUIRE(at13.size() == 1);
    }

    SECTION("negative shift moves anchors up") {
        collection.shiftAnchorsFrom(0, -2);
        
        // First image was at 5, now at 3
        auto at3 = collection.imagesAtLine(3);
        REQUIRE(at3.size() == 1);
    }

    SECTION("shift only affects images at or after line") {
        collection.shiftAnchorsFrom(8, 5);
        
        // Image at line 5 is unchanged
        auto at5 = collection.imagesAtLine(5);
        REQUIRE(at5.size() == 1);
        
        // Image at line 10 is shifted to 15
        auto at15 = collection.imagesAtLine(15);
        REQUIRE(at15.size() == 1);
    }
}

TEST_CASE("ImageCollection clear", "[image]") {
    ImageCollection collection;
    
    DocumentImage img;
    collection.addImage(img);
    collection.addImage(img);
    REQUIRE(collection.count() == 2);

    SECTION("clear removes all images") {
        collection.clear();
        REQUIRE(collection.isEmpty());
        REQUIRE(collection.count() == 0);
    }

    SECTION("ID counter resets after clear") {
        collection.clear();
        std::size_t newId = collection.addImage(img);
        REQUIRE(newId == 1);  // Should restart from 1
    }
}

TEST_CASE("ImageAlignment enum values", "[image]") {
    SECTION("alignment values exist") {
        REQUIRE(static_cast<int>(ImageAlignment::Left) == 0);
        REQUIRE(static_cast<int>(ImageAlignment::Center) == 1);
        REQUIRE(static_cast<int>(ImageAlignment::Right) == 2);
    }
}

TEST_CASE("DocumentImage border properties", "[image]") {
    DocumentImage img;
    
    SECTION("default border is transparent") {
        REQUIRE(img.borderWidth == 0.0f);
    }

    SECTION("border color can be set") {
        img.borderR = 255;
        img.borderG = 0;
        img.borderB = 0;
        img.borderA = 255;
        img.borderWidth = 2.0f;
        
        REQUIRE(img.borderR == 255);
        REQUIRE(img.borderWidth == 2.0f);
    }
}

TEST_CASE("DocumentImage alt text", "[image]") {
    DocumentImage img;
    
    SECTION("default alt text is empty") {
        REQUIRE(img.altText.empty());
    }

    SECTION("alt text can be set") {
        img.altText = "A photo of a sunset";
        REQUIRE(img.altText == "A photo of a sunset");
    }
}
