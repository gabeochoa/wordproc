#pragma once

#include <algorithm>
#include <cmath>

#include "../../vendor/afterhours/src/core/system.h"
#include "../editor/scroll_math.h"
#include "../editor/text_layout.h"
#include "../rl.h"
#include "../ui/input.h"
#include "../ui/theme.h"
#include "component_helpers.h"
#include "components.h"
#include "scrollbar_system.h"

namespace ecs {

// Click / drag text selection via hit testing. Single click places the
// caret, drag selects, shift-click extends, double-click selects the
// word, triple-click selects the paragraph. Dragging past the top or
// bottom edge auto-scrolls one line per frame. Typing replaces a
// selection through TextBuffer::insertChar -> deleteSelection.
// Runs after MenuUISystem/ScrollbarSystem; a press edge is consumed by
// prevDown_ tracking while a menu is open, so it never leaks through.
struct MouseSelectionSystem
    : public afterhours::System<DocumentComponent, CaretComponent, ScrollComponent,
                                LayoutComponent, MenuComponent, DialogState> {
    bool prevDown_ = false;
    bool dragging_ = false;
    // Anchor of the in-progress drag. Kept in the system (not in the
    // buffer) so a plain click leaves hasSelection()==false until the
    // drag actually moves.
    CaretPosition dragAnchor_{0, 0};
    double lastClickTime_ = -1.0;
    int clickCount_ = 0;
    float lastClickX_ = 0.0f;
    float lastClickY_ = 0.0f;

    CaretPosition hitTest(const TextBuffer& buffer, const LayoutComponent& layout,
                          const LayoutComponent::Rect& area, float mx, float my,
                          int scrollOffset, int fontSize, int lineHeight) const {
        float relY = my - area.y - static_cast<float>(theme::layout::TEXT_PADDING);
        int row = scrollOffset + static_cast<int>(std::floor(relY / static_cast<float>(lineHeight)));
        int lineCount = static_cast<int>(buffer.lineCount());
        if (row < 0) row = 0;
        if (row > lineCount - 1) row = lineCount - 1;
        float baseX = area.x + static_cast<float>(theme::layout::TEXT_PADDING);
        if (layout.showLineNumbers) baseX += layout.lineNumberGutterWidth;
        std::string line = buffer.lineString(static_cast<std::size_t>(row));
        auto measure = [fontSize](const std::string& prefix) {
            return static_cast<float>(
                afterhours::graphics::measure_text(prefix.c_str(), fontSize));
        };
        std::size_t col = hitTestColumn(line, mx - baseX, measure);
        return {static_cast<std::size_t>(row), col};
    }

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, ScrollComponent& scroll,
                       LayoutComponent& layout, MenuComponent& menu,
                       DialogState& dialogs, const float) override {
        bool anyMenuOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) { anyMenuOpen = true; break; }
        }
        bool down = input::isMouseButtonDown(0);
        bool pressed = down && !prevDown_;
        prevDown_ = down;

        if (anyMenuOpen || ScrollbarSystem::anyModalDialogOpen(dialogs)) {
            dragging_ = false;
            return;
        }

        TextStyle style = doc.buffer.textStyle();
        int fontSize = std::max(8, static_cast<int>(std::round(style.fontSize * layout.zoomLevel)));
        int lineHeight = fontSize + 4;
        LayoutComponent::Rect area = layout::effectiveTextArea(layout);
        float sbX = layout.textArea.x + layout.textArea.width - scroll_math::SCROLLBAR_WIDTH;

        auto mouse = input::getMousePosition();
        bool inTextArea = mouse.x >= area.x && mouse.x < sbX && mouse.y >= area.y &&
                          mouse.y < area.y + area.height;

        if (pressed && inTextArea) {
            bool shiftDown = test_input::is_key_down(afterhours::keys::LEFT_SHIFT) ||
                             test_input::is_key_down(afterhours::keys::RIGHT_SHIFT);
            CaretPosition pos = hitTest(doc.buffer, layout, area, mouse.x, mouse.y,
                                        scroll.offset, fontSize, lineHeight);

            // Multi-click: second press within 0.5s near the same point.
            double now = afterhours::graphics::get_time();
            if (now - lastClickTime_ < 0.5 &&
                std::abs(mouse.x - lastClickX_) < 4.0f &&
                std::abs(mouse.y - lastClickY_) < 4.0f) {
                clickCount_ += 1;
            } else {
                clickCount_ = 1;
            }
            lastClickTime_ = now;
            lastClickX_ = mouse.x;
            lastClickY_ = mouse.y;

            if (clickCount_ == 2) {
                doc.buffer.selectWordAt(pos);
                dragging_ = false;
            } else if (clickCount_ >= 3) {
                // No reset here: the next press in the same burst must
                // still count as >= 3 (resetting to 0 made a triple-click
                // end as a word selection). The time-gap branch above
                // resets to 1 after a 0.5s pause.
                doc.buffer.selectParagraphAt(pos);
                dragging_ = false;
            } else if (shiftDown) {
                if (!doc.buffer.hasSelection()) {
                    doc.buffer.setSelectionAnchor(doc.buffer.caret());
                }
                dragAnchor_ = doc.buffer.selectionStart();
                doc.buffer.setCaret(pos);
                doc.buffer.updateSelectionToCaret();
                dragging_ = true;
            } else {
                doc.buffer.clearSelection();
                doc.buffer.setCaret(pos);
                dragAnchor_ = pos;
                dragging_ = true;
            }
            caret::resetBlink(caret);
            return;
        }

        // Drag in progress: extend the selection, auto-scrolling past edges.
        if (dragging_) {
            if (!down) {
                dragging_ = false;
                // Collapse a zero-length drag to a plain caret.
                if (doc.buffer.hasSelection() &&
                    doc.buffer.getSelectedText().empty()) {
                    doc.buffer.clearSelection();
                }
                return;
            }
            if (!inTextArea) {
                // Dragged past the top/bottom edge: scroll one line per
                // frame toward the pointer, then recompute in bounds.
                int totalLines = static_cast<int>(doc.buffer.lineCount());
                int maxScroll = std::max(0, totalLines - scroll.visibleLines);
                if (mouse.y < area.y && scroll.offset > 0) {
                    scroll.offset -= 1;
                } else if (mouse.y > area.y + area.height && scroll.offset < maxScroll) {
                    scroll.offset += 1;
                }
            }
            float clampedY = std::clamp(mouse.y, area.y, area.y + area.height - 1.0f);
            CaretPosition pos = hitTest(doc.buffer, layout, area, mouse.x, clampedY,
                                        scroll.offset, fontSize, lineHeight);
            // First drag move after a plain click: open the selection at
            // the stored anchor. Shift-click already opened it.
            if (!doc.buffer.hasSelection()) {
                doc.buffer.setCaret(dragAnchor_);
                doc.buffer.setSelectionAnchor(dragAnchor_);
            }
            doc.buffer.setCaret(pos);
            doc.buffer.updateSelectionToCaret();
            caret::resetBlink(caret);
        }
    }
};

}  // namespace ecs
