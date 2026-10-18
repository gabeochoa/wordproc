#pragma once

#include <algorithm>

#include "../../vendor/afterhours/src/core/system.h"
#include "../editor/scroll_math.h"
#include "../rl.h"
#include "../ui/input.h"
#include "components.h"

namespace ecs {

// Win95 vertical scrollbar interaction: drag the thumb, click the track
// to page, press-hold the arrows to auto-repeat. Runs in the update
// phase, after MenuUISystem, so a menu that is open this frame wins and
// a click that closed a menu last frame is swallowed, never double-fed.
struct ScrollbarSystem : public afterhours::System<DocumentComponent, ScrollComponent,
                                                    LayoutComponent, MenuComponent, DialogState> {
    // A menu was open when the previous frame's UI ran. A press on that
    // frame belonged to the menu (switch/close/activate) and must not
    // also move the scrollbar.
    bool prevMenuOpen_ = false;
    // Previous frame's left-button state. The press edge is derived from
    // the down transition, like the UI layer does: the injector's
    // just_pressed edge is already cleared by the time update systems run
    // for mouse_down / drag_to (press_frames=0), so polling it misses
    // those presses entirely.
    bool prevDown_ = false;

    static bool anyModalDialogOpen(const DialogState& dialogs) {
        return dialogs.showContextMenu || dialogs.showFindDialog ||
               dialogs.showAboutDialog || dialogs.showHelpWindow ||
               dialogs.showCommentDialog || dialogs.showTemplateDialog ||
               dialogs.showSpellingDialog || dialogs.showAutoCorrectDialog ||
               dialogs.showWordCountDialog || dialogs.showTabWidthDialog ||
               dialogs.showSettingsDialog || dialogs.showBookmarkListDialog ||
               dialogs.showSaveAsDialog || dialogs.showPageSetup;
    }

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       ScrollComponent& scroll, LayoutComponent& layout,
                       MenuComponent& menu, DialogState& dialogs,
                       const float) override {
        bool anyMenuOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) { anyMenuOpen = true; break; }
        }
        bool down = input::isMouseButtonDown(0);
        bool pressed = down && !prevDown_;
        bool released = !down && prevDown_;
        prevDown_ = down;
        bool swallow = pressed && prevMenuOpen_;
        prevMenuOpen_ = anyMenuOpen;

        int totalLines = static_cast<int>(doc.buffer.lineCount());
        int visible = scroll.visibleLines > 0 ? scroll.visibleLines : 20;
        int maxScroll = std::max(0, totalLines - visible);

        float sbWidth = scroll_math::SCROLLBAR_WIDTH;
        float sbX = layout.textArea.x + layout.textArea.width - sbWidth;
        float sbY = layout.textArea.y;
        float sbHeight = layout.textArea.height;
        auto g = scroll_math::scrollbarGeometry(sbY, sbHeight, totalLines, visible,
                                               maxScroll, scroll.offset);
        auto clampOffset = [&]() {
            if (scroll.offset < 0) scroll.offset = 0;
            if (scroll.offset > maxScroll) scroll.offset = maxScroll;
        };

        auto mouse = input::getMousePosition();

        // A menu or modal owns this press: cancel any in-progress
        // scrollbar gesture and do nothing else.
        if (anyMenuOpen || anyModalDialogOpen(dialogs) || swallow) {
            scroll.scrollbarDragging = false;
            scroll_math::arrowRepeatRelease(scroll.scrollbarRepeatDir);
            return;
        }
        if (released && scroll.scrollbarDragging) {
            // Apply the drag's final position on release: if the move
            // frame was missed (drag phases racing the auto-release),
            // the drop still lands where the pointer is.
            float thumbY = scroll.scrollbarDragStartThumbY +
                           (mouse.y - scroll.scrollbarDragStartY);
            scroll.offset = scroll_math::offsetForThumbY(thumbY, g.trackY,
                                                         g.trackHeight, g.thumbHeight,
                                                         maxScroll);
            if (scroll.offset < 0) scroll.offset = 0;
            if (scroll.offset > maxScroll) scroll.offset = maxScroll;
        }
        if (released) {
            scroll.scrollbarDragging = false;
            scroll_math::arrowRepeatRelease(scroll.scrollbarRepeatDir);
        }

        // Thumb drag in progress: move the thumb with the pointer.
        if (scroll.scrollbarDragging) {
            if (!down) {
                scroll.scrollbarDragging = false;
                return;
            }
            float thumbY = scroll.scrollbarDragStartThumbY + (mouse.y - scroll.scrollbarDragStartY);
            scroll.offset = scroll_math::offsetForThumbY(thumbY, g.trackY, g.trackHeight,
                                                         g.thumbHeight, maxScroll);
            clampOffset();
            return;
        }

        // Arrow hold: auto-repeat while the button stays down.
        if (down && scroll.scrollbarRepeatDir != 0) {
            double now = afterhours::graphics::get_time();
            if (scroll_math::arrowRepeatDue(scroll.scrollbarRepeatDir,
                                             scroll.scrollbarRepeatNext, now)) {
                scroll.offset += scroll.scrollbarRepeatDir;
                clampOffset();
            }
            return;
        }

        if (!pressed) return;
        if (mouse.x < sbX || mouse.x > sbX + sbWidth) return;
        if (mouse.y < sbY || mouse.y > sbY + sbHeight) return;

        double now = afterhours::graphics::get_time();
        if (mouse.y < g.trackY) {
            // Up arrow: one line now, then auto-repeat while held.
            scroll.offset -= 1;
            clampOffset();
            scroll_math::arrowRepeatPress(scroll.scrollbarRepeatDir,
                                           scroll.scrollbarRepeatNext, -1, now);
        } else if (mouse.y > g.trackY + g.trackHeight) {
            // Down arrow: one line now, then auto-repeat while held.
            scroll.offset += 1;
            clampOffset();
            scroll_math::arrowRepeatPress(scroll.scrollbarRepeatDir,
                                           scroll.scrollbarRepeatNext, 1, now);
        } else if (g.thumbVisible && mouse.y >= g.thumbY &&
                   mouse.y <= g.thumbY + g.thumbHeight) {
            // Thumb: start a drag.
            scroll.scrollbarDragging = true;
            scroll.scrollbarDragStartY = mouse.y;
            scroll.scrollbarDragStartThumbY = g.thumbY;
            scroll.scrollbarDragStartOffset = scroll.offset;
        } else if (g.thumbVisible && mouse.y < g.thumbY) {
            // Track above the thumb: page up.
            scroll.offset -= visible;
            clampOffset();
        } else if (g.thumbVisible && mouse.y > g.thumbY + g.thumbHeight) {
            // Track below the thumb: page down.
            scroll.offset += visible;
            clampOffset();
        }
    }
};

}  // namespace ecs
