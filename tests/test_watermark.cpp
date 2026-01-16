#include "../src/editor/document_settings.h"
#include "catch2/catch.hpp"

TEST_CASE("Watermark struct", "[document_settings][watermark]") {
    Watermark wm;
    
    SECTION("default values") {
        REQUIRE(wm.type == WatermarkType::None);
        REQUIRE(wm.text.empty());
        REQUIRE(wm.imagePath.empty());
        REQUIRE(wm.opacity == Approx(0.3f));
        REQUIRE(wm.rotation == Approx(-45.0f));
        REQUIRE(wm.scale == Approx(1.0f));
        REQUIRE(wm.fontSize == 72);
    }
    
    SECTION("isEnabled returns false for None type") {
        REQUIRE_FALSE(wm.isEnabled());
    }
    
    SECTION("isEnabled returns true for Text type") {
        wm.type = WatermarkType::Text;
        wm.text = "DRAFT";
        REQUIRE(wm.isEnabled());
    }
    
    SECTION("isEnabled returns true for Image type") {
        wm.type = WatermarkType::Image;
        wm.imagePath = "/path/to/watermark.png";
        REQUIRE(wm.isEnabled());
    }
}

TEST_CASE("Text watermark configuration", "[document_settings][watermark]") {
    Watermark wm;
    wm.type = WatermarkType::Text;
    wm.text = "CONFIDENTIAL";
    
    SECTION("can set text content") {
        REQUIRE(wm.text == "CONFIDENTIAL");
    }
    
    SECTION("can set opacity") {
        wm.opacity = 0.5f;
        REQUIRE(wm.opacity == Approx(0.5f));
    }
    
    SECTION("can set rotation") {
        wm.rotation = 0.0f;  // Horizontal
        REQUIRE(wm.rotation == Approx(0.0f));
    }
    
    SECTION("can set color") {
        wm.color = TextColors::Red;
        REQUIRE(wm.color.r == 200);
    }
    
    SECTION("can set font") {
        wm.font = "EBGaramond-Regular";
        wm.fontSize = 48;
        REQUIRE(wm.font == "EBGaramond-Regular");
        REQUIRE(wm.fontSize == 48);
    }
}

TEST_CASE("Image watermark configuration", "[document_settings][watermark]") {
    Watermark wm;
    wm.type = WatermarkType::Image;
    wm.imagePath = "/path/to/logo.png";
    
    SECTION("can set image path") {
        REQUIRE(wm.imagePath == "/path/to/logo.png");
    }
    
    SECTION("can set scale") {
        wm.scale = 0.5f;  // Half size
        REQUIRE(wm.scale == Approx(0.5f));
    }
    
    SECTION("can set opacity for images") {
        wm.opacity = 0.2f;  // Very faint
        REQUIRE(wm.opacity == Approx(0.2f));
    }
}

TEST_CASE("DocumentSettings includes watermark", "[document_settings][watermark]") {
    DocumentSettings settings;
    
    SECTION("watermark is disabled by default") {
        REQUIRE_FALSE(settings.watermark.isEnabled());
    }
    
    SECTION("can configure watermark through settings") {
        settings.watermark.type = WatermarkType::Text;
        settings.watermark.text = "DRAFT";
        settings.watermark.opacity = 0.4f;
        
        REQUIRE(settings.watermark.isEnabled());
        REQUIRE(settings.watermark.text == "DRAFT");
    }
}

TEST_CASE("WatermarkType enum", "[document_settings][watermark]") {
    SECTION("None is default") {
        WatermarkType type = WatermarkType::None;
        REQUIRE(type == WatermarkType::None);
    }
    
    SECTION("can be Text") {
        WatermarkType type = WatermarkType::Text;
        REQUIRE(type == WatermarkType::Text);
    }
    
    SECTION("can be Image") {
        WatermarkType type = WatermarkType::Image;
        REQUIRE(type == WatermarkType::Image);
    }
}
