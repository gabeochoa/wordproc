# Status Bar Migration to Afterhours

## Current State

The status bar is drawn with raw raylib calls in `EditorRenderSystem::for_each_with` (lines 782-837 of `render_system.h`). It renders:

- Background rectangle with raised border
- Left section: "Page 1  Sec 1", "{line}/{total}", "At {X}\"", "Ln {n}  Col {n}"
- Right section: clock ("01:34 AM"), dimmed indicators ("REC", "MRK", "EXT", "OVR")

All text is drawn with `drawTextWithRegistry()` (which also registers text for e2e `expect_text` validation). Layout is manually calculated with pixel offsets.

Total: ~55 lines of raw raylib draw code.

## What Needs to Happen

1. **Create a `StatusBarSystem`** as an afterhours update system (like `ToolbarRenderSystem`).
2. **Build the status bar with afterhours UI primitives**: A horizontal `div` with `FlexDirection::Row`, containing labeled sections. Use `JustifyContent::SpaceBetween` for left/right alignment.
3. **Wire up document data**: Query `DocumentComponent` for caret position, line count, stats. Query `LayoutComponent` for screen dimensions and focus mode.
4. **Register text for e2e**: Afterhours UI text is already registered via `registerVisibleText` in the rendering pipeline. Verify this works for `expect_text` commands.
5. **Remove the raw drawing code** from `EditorRenderSystem` (lines 782-837).
6. **Skip in focus mode**: Already gated by `if (!layout.focusMode)`.

## Does It Need Afterhours Changes?

**No.** The status bar is a simple horizontal layout with text labels. Afterhours `div()`, `button()`, and `with_label()` cover this entirely. The Win95 raised border effect is available via `BevelStyle` in the current afterhours version.

One minor consideration: the status bar needs to update its labels every frame (caret position changes, clock ticks). Since afterhours IMM UI recreates entities every frame, this is natural.

## How to Validate

1. **E2E `expect_text` checks**: Several tests validate status bar content indirectly (e.g., `pass_expect_text_document` checks for rendered text). Add explicit `expect_text "Page 1"` and `expect_text "Ln 1"` checks if not already present.
2. **Screenshot comparison**: The status bar is visible in every screenshot. Compare before/after for correct layout, text content, and Win95 styling (raised border, gray background).
3. **Focus mode**: Verify status bar disappears when focus mode is toggled.
4. **Dynamic content**: Type text, move caret, verify Ln/Col updates in screenshots.

## Open Questions

1. **Clock display**: Should the clock still use `std::localtime` or should it be driven by a component? In test mode the clock shows real time which makes screenshots non-deterministic.
2. **Status indicators (REC/MRK/EXT/OVR)**: These are always dimmed/inactive. Are they planned to be functional, or should they be removed to simplify?
3. **Text registration**: Afterhours renders text via its own pipeline. Does `HasLabel` text automatically get registered with `test_input::registerVisibleText`? If not, we need a system that walks UI entities with `HasLabel` and registers their text.
