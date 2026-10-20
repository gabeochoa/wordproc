#pragma once

// Startup warm pass for menu text measurement (Perf Round 6).
//
// Round 4 measured first menu open at +2.0 ms one-time (reopen free);
// Round 5's MeasureCache fronts graphics::measure_text on the document
// render path, which the menu never calls — so first-open stayed cold
// (~1.2–1.9 ms/menu, re-measured this round). The dropdown measures
// through the UI-font path instead: MenuUISystem's own MeasureUIText
// calls plus autolayout/renderer measures of every item label and
// shortcut. Those bottom out in the backend measure_memo (persistent,
// never pruned), so measuring each string once here makes the first
// open hit the memo. Note the app-facing TextMeasureCache in front
// of it prunes entries unused for 120 frames — warming only that
// cache would evaporate before a real user opens a menu — which is
// why this pass goes through the persistent memo-level APIs.
//
// Call after every createMenuBar: at startup in app_init (fonts are
// loaded by then), and at the two rebuild sites (refreshMenuBar and
// the MenuUISystem rebuild check), which introduce strings the
// startup pass never saw (new recent paths, new document name).
// A rebuild warm is all memo hits for unchanged strings.
//
// Cost: ~1,000 measures of ~200 strings (4 mark-prefix variants
// each) plus one offscreen draw of each unique drawn string.
// Timed by the SCOPED_TIMERs below; the round-6 budget is <5 ms,
// and cold-open wall must not regress >2 ms.

#include <string>
#include <unordered_set>
#include <vector>

#include "../util/logging.h"
#include "menu_geometry.h"
#include "menu_setup.h"
#include "theme.h"

namespace menu_warm {

inline void warmMenuMeasures(const std::vector<win95::Menu>& menus) {
    if (!theme::UI_FONT_LOADED) return;
    SCOPED_TIMER("Warm menu measure cache");
    menu_geometry::forEachMenuWarmString(
        menus, [](const std::string& text, menu_geometry::MenuWarmSite site) {
            switch (site) {
                case menu_geometry::MenuWarmSite::HeaderWidth:
                case menu_geometry::MenuWarmSite::MnemonicWidth:
                    // Same call MenuUISystem makes for header widths
                    // (base size 16, Settings-scaled inside MeasureUIText).
                    (void)theme::MeasureUIText(text.c_str(), 16);
                    break;
                case menu_geometry::MenuWarmSite::Drawn:
                    // The size autolayout + the renderer measure/draw at:
                    // default font, DRAWN_FONT_SIZE_PX, Proportional
                    // (no Settings scale). Shortcuts measure here too:
                    // their boxes are sized from this call.
                    (void)afterhours::measure_text(
                        theme::UI_FONT, text.c_str(),
                        menu_geometry::DRAWN_FONT_SIZE_PX, 1.0f);
                    break;
                default: break;
            }
        });
}

// Glyph-raster warm pass (Perf Round 6, second half of the fix).
//
// Measure-warming alone did not fix first-open (Format stayed at
// ~2.4 ms): the remaining ~1.6 ms is fontstash glyph rasterization
// on first DRAW — fonsTextAdvance (the measure path) only computes
// advances, it never rasterizes, while fonsDrawText rasterizes each
// new glyph into the atlas. So draw every string the dropdown will
// draw, once, at the exact size it draws at
// (DRAWN_FONT_SIZE_PX), offscreen with alpha 0: the glyphs
// rasterize, the pixels paint nothing. Only Drawn-site strings are
// drawn (the MeasureUIText sizes are measure-only, never drawn);
// exact duplicates are drawn once.
// Returns false when there was nothing to draw (no menus / no font
// yet) so the caller can retry on a later frame. Call once, on the
// second frame — after the first paint, so first paint pays none of
// it — and before any menu can open (a click takes longer than one
// frame). Must run inside the draw pass (app_frame, after the
// systems run, before end_drawing).
inline bool warmMenuDraws(const std::vector<win95::Menu>& menus) {
    if (menus.empty() || !theme::UI_FONT_LOADED) return false;
    SCOPED_TIMER("Warm menu glyph draw");
    std::unordered_set<std::string> drawn;
    menu_geometry::forEachMenuWarmString(
        menus, [&](const std::string& text, menu_geometry::MenuWarmSite site) {
            if (site != menu_geometry::MenuWarmSite::Drawn) return;
            if (!drawn.insert(text).second) return;
            afterhours::draw_text_ex(
                theme::UI_FONT, text.c_str(), {-10000.0f, -10000.0f},
                menu_geometry::DRAWN_FONT_SIZE_PX, 1.0f,
                afterhours::Color{0, 0, 0, 0});
        });
    return true;
}

}  // namespace menu_warm
