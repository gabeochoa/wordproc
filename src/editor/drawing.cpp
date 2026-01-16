#include "drawing.h"

#include <algorithm>

std::size_t DrawingCollection::addDrawing(const DocumentDrawing& drawing) {
    DocumentDrawing drw = drawing;
    drw.id = nextId_++;
    drawings_.push_back(drw);
    return drw.id;
}

DocumentDrawing* DrawingCollection::getDrawing(std::size_t id) {
    auto it = std::find_if(drawings_.begin(), drawings_.end(),
                           [id](const DocumentDrawing& drw) { return drw.id == id; });
    return it != drawings_.end() ? &(*it) : nullptr;
}

const DocumentDrawing* DrawingCollection::getDrawing(std::size_t id) const {
    auto it = std::find_if(drawings_.begin(), drawings_.end(),
                           [id](const DocumentDrawing& drw) { return drw.id == id; });
    return it != drawings_.end() ? &(*it) : nullptr;
}

bool DrawingCollection::removeDrawing(std::size_t id) {
    auto it = std::find_if(drawings_.begin(), drawings_.end(),
                           [id](const DocumentDrawing& drw) { return drw.id == id; });
    if (it != drawings_.end()) {
        drawings_.erase(it);
        return true;
    }
    return false;
}

std::vector<DocumentDrawing*> DrawingCollection::drawingsAtLine(std::size_t line) {
    std::vector<DocumentDrawing*> result;
    for (auto& drw : drawings_) {
        if (drw.anchorLine == line) {
            result.push_back(&drw);
        }
    }
    return result;
}

std::vector<const DocumentDrawing*> DrawingCollection::drawingsAtLine(std::size_t line) const {
    std::vector<const DocumentDrawing*> result;
    for (const auto& drw : drawings_) {
        if (drw.anchorLine == line) {
            result.push_back(&drw);
        }
    }
    return result;
}

std::vector<const DocumentDrawing*> DrawingCollection::drawingsInRange(
    std::size_t startLine, std::size_t endLine) const {
    std::vector<const DocumentDrawing*> result;
    for (const auto& drw : drawings_) {
        if (drw.anchorLine >= startLine && drw.anchorLine <= endLine) {
            result.push_back(&drw);
        }
    }
    return result;
}

void DrawingCollection::shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta) {
    for (auto& drw : drawings_) {
        if (drw.anchorLine >= line) {
            if (linesDelta < 0 && static_cast<std::size_t>(-linesDelta) > drw.anchorLine) {
                drw.anchorLine = 0;
            } else {
                drw.anchorLine = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(drw.anchorLine) + linesDelta);
            }
        }
    }
}

void DrawingCollection::clear() {
    drawings_.clear();
    nextId_ = 1;
}
