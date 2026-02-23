# Remove Per-Frame Full-Document Text Registration

**Date:** 2026-02-23
**Type:** Performance — eliminate unnecessary O(n) work per frame
**Impact:** Removes O(document_lines) iteration from every frame in production builds

## Problem

`StatusBarSystem::for_each_with()` in `src/ecs/status_bar_system.h` (lines 77–85) iterates over every line in the document every frame to register text for E2E testing:

```cpp
size_t lineCount = doc.buffer.lineCount();
for (size_t i = 0; i < lineCount; ++i) {
    auto view = doc.buffer.lineView(i);
    if (view && view.length > 0) {
        test_input::register_visible_text(std::string(view.data, view.length));
    } else if (!view && doc.buffer.lineSpan(i).length > 0) {
        test_input::register_visible_text(doc.buffer.lineString(i));
    }
}
```

This has two problems:

1. **Performance:** For a 10,000-line document, this does 10k `lineView()` calls + 10k `register_visible_text()` calls every frame. The status bar has no business iterating document lines.

2. **Wrong location:** Document text registration should happen where document text is actually rendered — in `renderTextBuffer()` inside `render_system.h`, which already iterates visible lines and calls `test_input::register_visible_text(displayLine)` (line 478).

The status bar loop registers ALL lines (including off-screen ones), while `renderTextBuffer` only registers visible lines. The status bar version is both more expensive AND registers text that isn't actually visible.

## Proposed Solution

### Option A: Guard with test mode check (minimal change)

```cpp
if (testComp && testComp->enabled) {
    // ... existing loop ...
}
```

This requires querying for `TestConfigComponent`, but avoids the work in production.

### Option B: Remove entirely (recommended)

Delete the loop from `StatusBarSystem`. Document text is already registered by `renderTextBuffer` for visible lines. If E2E tests need to validate text that's scrolled off-screen, they should scroll to it first (which is the correct behavior anyway — you can't validate what isn't rendered).

Verify that existing E2E tests still pass without this loop. If any test relies on finding off-screen text, fix the test to scroll first.

## Files to Change

| File | Change |
|------|--------|
| `src/ecs/status_bar_system.h` | Remove lines 77–85 (the document text iteration loop) |

## Risks

- Some E2E tests may rely on `register_visible_text` finding off-screen content. These tests would need updating to scroll to the relevant content first.
- `validate text_contains_*` checks in E2E tests search registered visible text — if a test validates text from a line that's not currently rendered, it would fail.

## Migration Strategy

1. Remove the loop
2. Run all E2E tests
3. Fix any tests that relied on off-screen text being registered (by adding `scroll` or `key DOWN` commands before the validation)
