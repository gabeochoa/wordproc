#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Image layout modes for text wrapping
enum class ImageLayoutMode {
    Inline,     // Image is placed inline with text, like a character
    WrapSquare, // Text wraps around the image bounding box
    WrapTight,  // Text wraps tightly around image contours (not fully implemented)
    BreakText,  // Image breaks text, no text appears beside it
    Behind,     // Image appears behind text
    InFront     // Image appears in front of text
};

// Image horizontal alignment within its container
enum class ImageAlignment {
    Left,
    Center,
    Right
};

// Get display name for layout mode
inline const char* imageLayoutModeName(ImageLayoutMode mode) {
    switch (mode) {
        case ImageLayoutMode::Inline: return "Inline with Text";
        case ImageLayoutMode::WrapSquare: return "Square Wrap";
        case ImageLayoutMode::WrapTight: return "Tight Wrap";
        case ImageLayoutMode::BreakText: return "Break Text";
        case ImageLayoutMode::Behind: return "Behind Text";
        case ImageLayoutMode::InFront: return "In Front of Text";
        default: return "Inline";
    }
}

// Image data stored in a document
struct DocumentImage {
    // Image source
    std::string filename;    // Original filename (for display and re-loading)
    std::string base64Data;  // Base64-encoded image data (for embedded images)
    bool isEmbedded = true;  // True if image data is embedded, false if external link
    
    // Position in document
    std::size_t anchorLine = 0;    // Line number where image is anchored
    std::size_t anchorColumn = 0;  // Column in line (for inline mode)
    
    // Dimensions
    float originalWidth = 0.0f;   // Original image width in pixels
    float originalHeight = 0.0f;  // Original image height in pixels
    float displayWidth = 0.0f;    // Current display width (may be scaled)
    float displayHeight = 0.0f;   // Current display height (may be scaled)
    
    // Layout
    ImageLayoutMode layoutMode = ImageLayoutMode::Inline;
    ImageAlignment alignment = ImageAlignment::Left;
    
    // Offsets for positioned images (non-inline)
    float offsetX = 0.0f;  // Horizontal offset from anchor
    float offsetY = 0.0f;  // Vertical offset from anchor
    
    // Margins (space around image when text wraps)
    float marginTop = 4.0f;
    float marginBottom = 4.0f;
    float marginLeft = 4.0f;
    float marginRight = 4.0f;
    
    // Border
    float borderWidth = 0.0f;      // 0 = no border
    unsigned char borderR = 0;
    unsigned char borderG = 0;
    unsigned char borderB = 0;
    unsigned char borderA = 255;
    
    // Alt text for accessibility
    std::string altText;
    
    // Unique identifier for this image in the document
    std::size_t id = 0;
    
    // Helper methods
    bool hasEmbeddedData() const { return isEmbedded && !base64Data.empty(); }
    bool hasExternalSource() const { return !isEmbedded && !filename.empty(); }
    
    float aspectRatio() const {
        if (originalHeight <= 0.0f) return 1.0f;
        return originalWidth / originalHeight;
    }
    
    // Set display size maintaining aspect ratio
    void setDisplayWidth(float width) {
        displayWidth = width;
        displayHeight = width / aspectRatio();
    }
    
    void setDisplayHeight(float height) {
        displayHeight = height;
        displayWidth = height * aspectRatio();
    }
    
    // Reset to original size
    void resetSize() {
        displayWidth = originalWidth;
        displayHeight = originalHeight;
    }
    
    // Get bounding box including margins
    struct Bounds {
        float x, y, width, height;
    };
    
    Bounds getBounds(float anchorX, float anchorY) const {
        float totalWidth = displayWidth + marginLeft + marginRight + (borderWidth * 2);
        float totalHeight = displayHeight + marginTop + marginBottom + (borderWidth * 2);
        
        float x = anchorX + offsetX - marginLeft - borderWidth;
        float y = anchorY + offsetY - marginTop - borderWidth;
        
        // Apply alignment
        if (alignment == ImageAlignment::Center) {
            // This would need container width for proper centering
        } else if (alignment == ImageAlignment::Right) {
            // This would need container width for proper right-alignment
        }
        
        return {x, y, totalWidth, totalHeight};
    }
};

// Image collection in a document
class ImageCollection {
public:
    ImageCollection() = default;
    
    // Add an image at the specified anchor position
    std::size_t addImage(const DocumentImage& image);
    
    // Get image by ID
    DocumentImage* getImage(std::size_t id);
    const DocumentImage* getImage(std::size_t id) const;
    
    // Remove image by ID
    bool removeImage(std::size_t id);
    
    // Get all images
    const std::vector<DocumentImage>& images() const { return images_; }
    std::vector<DocumentImage>& images() { return images_; }
    
    // Get images anchored at a specific line
    std::vector<DocumentImage*> imagesAtLine(std::size_t line);
    std::vector<const DocumentImage*> imagesAtLine(std::size_t line) const;
    
    // Get images that affect a specific line range (for text wrap calculation)
    std::vector<const DocumentImage*> imagesInRange(std::size_t startLine, std::size_t endLine) const;
    
    // Update anchor positions after text edits
    void shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta);
    
    // Clear all images
    void clear();
    
    // Count
    std::size_t count() const { return images_.size(); }
    bool isEmpty() const { return images_.empty(); }
    
private:
    std::vector<DocumentImage> images_;
    std::size_t nextId_ = 1;
};
