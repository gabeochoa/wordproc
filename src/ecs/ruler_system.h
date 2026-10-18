#pragma once

#include "../../vendor/afterhours/src/core/system.h"
#include "../ui/input.h"
#include "component_helpers.h"
#include "components.h"
#include "scrollbar_system.h"

namespace ecs {

// Drag the ruler's indent markers to set the caret paragraph's
// indents through the command layer: live updates on move frames via
// TextBuffer::setLineIndents (no history), one undo step for the whole
// drag committed on release via commitParagraphIndents. A press that
// closed a menu last frame is swallowed (same rule as ScrollbarSystem)
// so it never starts a drag, and a menu/modal opening mid-drag
// restores the pre-drag indents and cancels.
struct RulerSystem : public afterhours::System<DocumentComponent, LayoutComponent,
                                             MenuComponent, DialogState> {
    bool prevDown_ = false;
    bool prevMenuOpen_ = false;
    bool dragging_ = false;
    ruler_math::Marker dragMarker_ = ruler_math::Marker::None;
    std::size_t dragRow_ = 0;
    ParagraphIndents oldIndents_{};
    ParagraphIndents currentIndents_{};

    void cancelDrag(DocumentComponent& doc) {
        if (dragging_) {
            doc.buffer.setLineIndents(dragRow_, oldIndents_);
            dragging_ = false;
        }
    }

    void applyFromMouse(DocumentComponent& doc, const ruler_math::Geometry& g,
                        float mouseX) {
        ParagraphIndents next = currentIndents_;
        switch (dragMarker_) {
            case ruler_math::Marker::Left: {
                next.left = ruler_math::indentFromX(g, mouseX);
                // Dragging left moves the first line along with it;
                // keep the first line inside the margin.
                if (next.firstLine < -next.left) next.firstLine = -next.left;
                break;
            }
            case ruler_math::Marker::FirstLine:
                next.firstLine = ruler_math::firstLineIndentFromX(
                    g, mouseX, currentIndents_.left);
                break;
            case ruler_math::Marker::Right: {
                next.right = ruler_math::rightIndentFromX(g, mouseX);
                int span = static_cast<int>(
                    std::lround(g.textSpanWidth));
                if (next.right > span - next.left)
                    next.right = std::max(0, span - next.left);
                break;
            }
            case ruler_math::Marker::None:
                break;
        }
        if (!(next == currentIndents_)) {
            currentIndents_ = next;
            doc.buffer.setLineIndents(dragRow_, next);
            doc.isDirty = true;
        }
    }

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       LayoutComponent& layout, MenuComponent& menu,
                       DialogState& dialogs, const float) override {
        bool anyMenuOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) {
                anyMenuOpen = true;
                break;
            }
        }
        bool down = input::isMouseButtonDown(0);
        bool pressed = down && !prevDown_;
        bool released = !down && prevDown_;
        prevDown_ = down;
        bool swallow = pressed && prevMenuOpen_;
        prevMenuOpen_ = anyMenuOpen;

        if (anyMenuOpen || ScrollbarSystem::anyModalDialogOpen(dialogs) ||
            swallow) {
            cancelDrag(doc);
            return;
        }
        if (!ruler::visible(layout)) {
            cancelDrag(doc);
            return;
        }

        if (released && dragging_) {
            dragging_ = false;
            doc.buffer.commitParagraphIndents(dragRow_, oldIndents_,
                                              currentIndents_);
            return;
        }
        if (dragging_) {
            if (!down) {
                dragging_ = false;
                return;
            }
            ruler_math::Geometry g = ruler::geometry(layout);
            auto mouse = input::getMousePosition();
            applyFromMouse(doc, g, mouse.x);
            return;
        }

        if (!pressed) return;
        auto mouse = input::getMousePosition();
        ruler_math::Geometry g = ruler::geometry(layout);
        std::size_t row = doc.buffer.caret().row;
        ParagraphIndents indents = doc.buffer.lineIndents(row);
        ruler_math::Marker marker = ruler_math::hitTest(
            g, indents.left, indents.firstLine, indents.right, mouse.x,
            mouse.y);
        if (marker == ruler_math::Marker::None) return;
        dragging_ = true;
        dragMarker_ = marker;
        dragRow_ = row;
        oldIndents_ = indents;
        currentIndents_ = indents;
    }
};

}  // namespace ecs
