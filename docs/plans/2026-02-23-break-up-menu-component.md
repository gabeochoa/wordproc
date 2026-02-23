# Break Up the MenuComponent God Object

**Date:** 2026-02-23
**Type:** Refactoring — reduce component coupling, improve modularity
**Impact:** Reduces MenuComponent from ~70 fields to ~15, enables per-dialog system splitting

## Problem

`MenuComponent` in `src/ecs/components.h` holds state for 8 unrelated dialogs plus the menu bar itself:

- Menu bar state (menus, activeMenuIndex, lastClickedResult)
- Find/Replace dialog (showFindDialog, findReplaceMode, lastSearchTerm, replaceTerm, findOptions, findInputBuffer, replaceInputBuffer, findInputStr, replaceInputStr)
- Word Count dialog (showWordCountDialog)
- Comment dialog (showCommentDialog, pendingCommentStart, pendingCommentEnd, commentInputBuffer, commentInputStr)
- Template dialog (showTemplateDialog, templateInputBuffer, templateInputStr)
- Tab Width dialog (showTabWidthDialog, tabWidthInputBuffer, tabWidthInputStr)
- Page Setup dialog (showPageSetup, selectedPageSize, selectedOrientation, margin*Mm)
- Save As dialog (showSaveAsDialog, saveAsInputBuffer, saveAsInputStr)
- Bookmark List dialog (showBookmarkListDialog)
- Settings dialog (showSettingsDialog, uiScaleInputStr)
- Help window (showHelpWindow, helpScrollOffset)
- About dialog (showAboutDialog)
- Pending native dialog (pendingDialog enum)

Any system that needs to check "is a menu open?" pulls in all dialog state. Adding a new dialog means editing this already-large struct.

## Proposed Solution

Extract each dialog into its own component:

```cpp
struct MenuBarComponent : public afterhours::BaseComponent {
    std::vector<win95::Menu> menus;
    int activeMenuIndex = -1;
    int lastClickedResult = -1;
    int recentFilesCount = 0;

    int consumeClickedResult() { /* ... */ }

    enum class PendingDialog { None, Open, SaveAs };
    PendingDialog pendingDialog = PendingDialog::None;
};

struct FindDialogState : public afterhours::BaseComponent {
    bool visible = false;
    bool replaceMode = false;
    std::string lastSearchTerm;
    std::string replaceTerm;
    FindOptions options;
    std::string findInputStr;
    std::string replaceInputStr;
};

struct CommentDialogState : public afterhours::BaseComponent {
    bool visible = false;
    std::size_t pendingStart = 0;
    std::size_t pendingEnd = 0;
    std::string inputStr;
};

struct PageSetupDialogState : public afterhours::BaseComponent {
    bool visible = false;
    PageSize selectedPageSize = PageSize::Letter;
    PageOrientation selectedOrientation = PageOrientation::Portrait;
    int marginTopMm = 25;
    int marginBottomMm = 25;
    int marginLeftMm = 25;
    int marginRightMm = 25;
};

// Simple bool-only dialogs can share a pattern:
struct SimpleDialogState : public afterhours::BaseComponent {
    bool showAbout = false;
    bool showWordCount = false;
    bool showHelp = false;
    int helpScrollOffset = 0;
    bool showBookmarkList = false;
    bool showSettings = false;
    std::string uiScaleInputStr;
    bool showTemplate = false;
    std::string templateInputStr;
    bool showTabWidth = false;
    std::string tabWidthInputStr;
};
```

## Files to Change

| File | Change |
|------|--------|
| `src/ecs/components.h` | Split `MenuComponent` into `MenuBarComponent` + dialog components |
| `src/ecs/menu_ui_system.h` | Update queries and field references |
| `src/ecs/render_system.h` | Update `MenuSystem` and `handleMenuActionImpl` references |
| `src/ecs/input_system.h` | Update `KeyboardShortcutSystem` references to menu |
| `src/main.cpp` | Update component creation and `app_state` pointers |
| `src/testing/e2e_runner.cpp` | Update property getters that read dialog state |

## Risks

- Many files reference `MenuComponent` — this is a broad rename
- E2E test runner reads dialog state for `validate dialog_open=` — needs to check all dialog components
- `handleMenuActionImpl` accesses both menu bar state and dialog visibility — needs both components passed in

## Migration Strategy

1. Start by extracting `FindDialogState` (it has the most fields and is self-contained)
2. Verify E2E tests (`pass_dialog_find_replace`) still pass
3. Extract remaining dialogs one at a time
4. Rename `MenuComponent` to `MenuBarComponent` last (broadest rename)
