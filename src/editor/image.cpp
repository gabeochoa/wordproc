#include "image.h"

#include <algorithm>

std::size_t ImageCollection::addImage(const DocumentImage& image) {
    DocumentImage img = image;
    img.id = nextId_++;
    images_.push_back(img);
    return img.id;
}

DocumentImage* ImageCollection::getImage(std::size_t id) {
    auto it = std::find_if(images_.begin(), images_.end(),
                           [id](const DocumentImage& img) { return img.id == id; });
    return it != images_.end() ? &(*it) : nullptr;
}

const DocumentImage* ImageCollection::getImage(std::size_t id) const {
    auto it = std::find_if(images_.begin(), images_.end(),
                           [id](const DocumentImage& img) { return img.id == id; });
    return it != images_.end() ? &(*it) : nullptr;
}

bool ImageCollection::removeImage(std::size_t id) {
    auto it = std::find_if(images_.begin(), images_.end(),
                           [id](const DocumentImage& img) { return img.id == id; });
    if (it != images_.end()) {
        images_.erase(it);
        return true;
    }
    return false;
}

std::vector<DocumentImage*> ImageCollection::imagesAtLine(std::size_t line) {
    std::vector<DocumentImage*> result;
    for (auto& img : images_) {
        if (img.anchorLine == line) {
            result.push_back(&img);
        }
    }
    return result;
}

std::vector<const DocumentImage*> ImageCollection::imagesAtLine(std::size_t line) const {
    std::vector<const DocumentImage*> result;
    for (const auto& img : images_) {
        if (img.anchorLine == line) {
            result.push_back(&img);
        }
    }
    return result;
}

std::vector<const DocumentImage*> ImageCollection::imagesInRange(
    std::size_t startLine, std::size_t endLine) const {
    std::vector<const DocumentImage*> result;
    for (const auto& img : images_) {
        if (img.anchorLine >= startLine && img.anchorLine <= endLine) {
            result.push_back(&img);
        }
    }
    return result;
}

void ImageCollection::shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta) {
    for (auto& img : images_) {
        if (img.anchorLine >= line) {
            if (linesDelta < 0 && static_cast<std::size_t>(-linesDelta) > img.anchorLine) {
                img.anchorLine = 0;
            } else {
                img.anchorLine = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(img.anchorLine) + linesDelta);
            }
        }
    }
}

void ImageCollection::clear() {
    images_.clear();
    nextId_ = 1;
}
