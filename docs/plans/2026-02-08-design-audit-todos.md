# Design Audit TODOs

From the design audit conducted 2026-02-08, evaluated against Apple HIG, Win95, Sun JLF, and Material Design 3.
Consolidated from 4 audit passes. Triaged 2026-02-08.

## Yes — Doing

### Flagged by all 4 audits
- [ ] **1. Toolbar icons**: Replace single-letter labels (N, O, S, P, X, C, V, <, >) with pixel-art icons
- [x] **2. Vertical scroll bar**: ~~Add scrollbar to right edge of document area~~ ✅ Done (Win95-style proportional thumb)
- [x] **3. Toast notification stacking**: ~~Fix unlimited stacking, cap at 1-2 or use status bar~~ ✅ dismiss_all() prevents stacking
- [x] **4. Tooltips on toolbar buttons**: ~~Add hover tooltips ("New (Ctrl+N)", etc.)~~ ✅ Done (yellow tooltip on hover)

### Flagged by 3 audits
- [x] **5. Menu order + Help last**: ~~Reorder to File, Edit, View, Insert, Format, Tools, Table, Help. Merge Settings into Tools > Options~~ ✅ Done
- [ ] **6. Access keys / mnemonics**: Add underlined Alt+key letters to menu titles and items
- [x] **7. Formatting button active state**: ~~Make B/I/U/L/C/R/J clearly sunken/pressed when active~~ ✅ Already working (sunken bevel + pressed BG)
- [x] **9. Status bar green text**: ~~Change "Auto-saved" from green to normal text color~~ ✅ Uses text prefixes now
- [ ] **10. Keyboard shortcuts on all menus**: Show shortcuts beyond just File menu
- [x] **11. Color-only status feedback**: ~~Add icon or bold alongside color for auto-saved~~ ✅ Text prefixes: [saved], [error], [recovered]
- [ ] **12. Focus indicators**: Add visible keyboard focus rectangles on interactive elements

### Flagged by 2 audits
- [x] **13. Window control buttons**: ~~Add min/max/close to title bar~~ ✅ Done (_, o, X)
- [x] **14. Text area sunken border**: ~~Add 2px sunken bevel around document area~~ ✅ Already existed (util::drawSunkenBorder)
- [x] **15. Toolbar separators**: ~~Add etched vertical lines between button groups~~ ✅ Done (dark+light line pairs)
- [ ] **16. Status bar inconsistency**: Ensure full status bar layout in all views
- [ ] **17. Title bar font**: Use sans-serif system font instead of monospace
- [ ] **18. Selection highlight contrast**: Make selected text readable (white on navy)
- [x] **19. Insert menu too long**: ~~Move shapes into cascading submenu~~ ✅ Consolidated with "Shape:" prefix, removed redundant "Shape..." entry
- [ ] **20. Hover state on buttons**: Add visual feedback on mouse-over
- [ ] **21. Status bar abbreviation tooltips**: Add hover explanations for REC/MRK/EXT/OVR
- [ ] **22. Consistent spacing**: Normalize gaps between title/menu/toolbar/formatting/ruler

### Flagged by 1 audit
- [ ] **24. Context menus**: Add right-click menus in document area
- [ ] **25. Insert menu disabled states**: Gray out items that don't apply
- [x] **27. Title bar unsaved indicator**: ~~Make unsaved-changes more visible than asterisk~~ ✅ Shows [Modified]
- [ ] **28. Ruler alignment**: Fix inconsistent tick mark spacing
- [ ] **29. Ruler margin handles**: Add draggable indent/margin handles
- [ ] **33. Type hierarchy**: Define distinct font sizes/weights for title vs menus vs status
- [ ] **34. Drop shadows on menus**: Menu dropdowns should float visually above content
- [ ] **35. Responsive layout**: Adapt toolbar/menus when window is resized small
- [ ] **36. Motion/animation**: Subtle open/close transitions on menus, toasts
- [ ] **38. Progress indicators**: Add feedback for long operations (save, export)

## Maybe — Needs Investigation

- [ ] **8. Dropdown arrow glyph**: Replace "v" with ▼ — likely an afterhours default dropdown issue
- [ ] **26. Smart quotes rendering bug**: Shows `\?Hello\?` instead of curly quotes
- [ ] **30. Status bar interactivity**: Make REC/MRK/EXT/OVR clickable — maybe later
- [ ] **31. Button sizes too small**: Check WCAG guidelines in afterhours first
- [ ] **32. Drag handles on toolbars**: Don't support dragging yet anyway
- [ ] **37. Dark mode verification**: Not a priority right now, maybe later

## No — Not Doing

- ~~**23. Size grip**~~
- ~~**39. Menu item icons**~~
- ~~**40. Splash screen**~~ — fix perf instead if startup is slow

## Strengths (Keep)

- Status bar layout (Page/Sec/Line/Col + REC/MRK/EXT/OVR + clock) is faithful to Word 6.0
- Menu bar spacing uses measured text widths — looks professional
- File menu dropdown has proper ellipsis, right-aligned shortcuts, and separator grouping
- 3D bevel borders on toolbar buttons and containers match Win95 style
- Win95 color palette is accurate (gray 192, title blue 0/0/128, white text)
- Layout order is correct: Title > Menu > Toolbar > Formatting > Ruler > Document > Status
- Selection highlight uses correct COLOR_HIGHLIGHT (0,0,128) with white text
- Title bar shows document name and modification state
- Double-click word selection works
- Proper ellipsis convention on dialog-launching items

## Score: 4-5/10

Skeleton is solid. Fixing the "Yes" items should bring this to 8+.

---

# audit-win95-design: Screenshot Audit (2026-02-08)

Reviewed 16+ screenshots from `tests/screenshots/` and `screenshots/` directories, covering: default empty state, text typing, bold/italic/underline formatting, menu dropdowns (File, View, Insert), toolbar states (redo, cut, align), multiline editing, indentation, lists, status bar, table insert, bookmark, text color, page break, double-click selection, smart quotes, dark mode, and toast notification stacking.

## Audit Summary: Full Application UI

### Critical Issues (Fix Immediately)

1. **Toolbars — No icons**: Toolbar buttons use single-letter text labels (N, O, S, P, X, C, V, <, >) instead of standard 16x16 pixel-art icons. Per Win95 guidelines, toolbars should use recognizable graphic images (page=New, folder=Open, floppy=Save, printer=Print, scissors=Cut, clipboard=Copy, paste=Paste, undo arrow, redo arrow). Single letters are ambiguous and fail the "Directness" principle.
   → **Fix**: Replace all letter labels with standard Word 6.0 toolbar icons at 16x16 within 24x22 button frames.

2. **Windows — No window control buttons**: Title bar lacks the standard minimize, maximize/restore, and close buttons on the right side. Win95 guidelines require these as part of every primary window title bar.
   → **Fix**: Add minimize (underscore), maximize (square), and close (X) glyph buttons to the right side of the title bar. Use standard 16x14 button sizes.

3. **Windows — No vertical scroll bar**: Document area has no scroll bar even when content could exceed the viewport. Win95 requires scroll bars to always be present (enabled when content overflows, dimmed when it fits).
   → **Fix**: Add a standard vertical scroll bar to the right edge of the document area with proportional thumb sizing.

4. **Feedback — Toast notification stacking**: Screenshots `toolbar_redo_enabled.png` and `toolbar_after_cut.png` show 10+ "[i] Auto-saved" toast notifications stacking vertically, completely obscuring the toolbar and formatting bar. This violates the "User in Control" principle — the user cannot dismiss or prevent these toasts from blocking their work.
   → **Fix**: Limit visible toasts to 1-2 max. Use the status bar for auto-save feedback instead (per Win95 convention, status bar displays operational state). Dismiss toasts automatically within 3 seconds.

5. **Menus — Help not last**: In multiple screenshots (pass_* series), menu order is: File, Edit, View, Format, Insert, Table, Help, Tools, Settings. Help must always be the rightmost menu per Win95 guidelines. "Settings" is non-standard.
   → **Fix**: Reorder to: File, Edit, View, Insert, Format, Tools, Table, Help. Merge "Settings" into Tools > Options.

### Major Issues (Fix Before Launch)

1. **Menus — Non-standard menu order**: Current order (File, Edit, View, Format, Insert, Table, Help, Tools, Settings) doesn't match the Win95 standard (File, Edit, View, Insert, Format, Tools, Table, Window, Help). Format appears before Insert; Tools appears after Help.
   → **Fix**: Reorder menus to match standard. If Window menu is not needed, omit it but keep Help last.

2. **Menus — No access keys (underlined letters)**: Menu bar items (File, Edit, View, etc.) show no underlined access key characters. Win95 requires every menu title and every menu item to have an access key (Alt+letter to open, then letter to select).
   → **Fix**: Add access key underlines: **F**ile, **E**dit, **V**iew, **I**nsert, F**o**rmat, **T**ools, Ta**b**le, **H**elp. Each menu item needs an access key too.

3. **Visual Design — Document area lacks sunken border**: The text editing area has no visible sunken bevel border. Win95 edit controls use a 2px sunken field border (sunken outer + sunken inner) to distinguish editable areas from the background.
   → **Fix**: Add `BevelStyle::Sunken` border around the document area using COLOR_BTNSHADOW and COLOR_BTNHIGHLIGHT.

4. **Toolbars — No visible separators**: Toolbar button groups (file ops vs. clipboard ops vs. navigation) have no visual separators. Win95 uses dark+light vertical line pairs with 8px padding between groups.
   → **Fix**: Add etched vertical separators between logical button groups.

5. **Toolbars — No tooltips**: Cannot confirm tooltips exist from screenshots. Win95 requires every toolbar button to have a tooltip describing its function.
   → **Fix**: Add tooltips to all toolbar buttons (e.g., "New (Ctrl+N)", "Open (Ctrl+O)").

6. **Controls — Dropdown arrow glyph**: Formatting bar dropdowns ("Normal v", "Times New Roman v", "10 v") use a lowercase "v" text character instead of a proper downward-pointing triangle glyph. This looks unprofessional and violates Win95 drop-down list conventions.
   → **Fix**: Replace "v" with a proper triangle glyph (▼) or draw a small filled triangle, visually separated from the label text.

7. **Status Bar — Inconsistent across UI states**: The migrated UI (pass_* screenshots) shows a proper Word 6.0-style status bar (Page/Sec/Ln/Col + REC/MRK/EXT/OVR + time), but some screenshots (e2e_* series) show a minimal status bar with only green "Auto-saved" text. The status bar content should be consistent.
   → **Fix**: Ensure the full status bar layout is always present. "Auto-saved" should appear as a transient message in the left status field, not replace the entire bar.

8. **Status Bar — Green text color**: "Auto-saved" text is rendered in green, which is not a Win95 system color. Status bar text should use COLOR_BTNTEXT (black, 0/0/0).
   → **Fix**: Use COLOR_BTNTEXT for all status bar text. If highlighting is needed, use bold or a brief flash.

9. **Visual Design — Formatting button active state**: When bold is active (pass_bold screenshot), the B button appears darker but the sunken/pressed visual state is subtle at the small button size. Win95 requires clearly distinguishable pressed states.
   → **Fix**: Ensure active formatting buttons use a clearly visible sunken border and darker background (COLOR_BTNSHADOW inset).

10. **Menus — Insert menu too long**: The Insert dropdown (visible in `05_bold_applied.png` and `e2e_bookmark.png`) has 20+ items spanning nearly the full screen height. Win95 recommends cascading menus for groups of related items.
    → **Fix**: Move Shape sub-items (Line, Rectangle, Circle, Ellipse, Arrow, Rounded Rectangle, Triangle) into a cascading "Shape" submenu.

### Minor Issues (Nice to Have)

1. **Visual Design — Title bar font**: Title bar text appears to use a monospace-like font rather than the system font (MS Sans Serif / Tahoma). Win95 title bars use the system caption font.
   → **Fix**: Use the system caption font for the title bar text.

2. **Visual Design — Ruler alignment**: Ruler tick marks and numbers have slightly inconsistent spacing in some screenshots.
   → **Fix**: Ensure ruler marks follow exact DPI-based inch/cm divisions.

3. **Menus — No keyboard shortcuts on all items**: While File menu shows Ctrl+N, Ctrl+O, many other menu items (across View, Insert, Format) lack keyboard shortcut annotations. Win95 encourages showing shortcuts for common operations.
   → **Fix**: Add shortcut annotations for frequently-used commands across all menus.

4. **Status Bar — Indicator interactivity**: REC, MRK, EXT, OVR indicators should be clickable to toggle their modes. Cannot confirm this from screenshots alone.
   → **Fix**: Make status bar mode indicators clickable toggles.

5. **Windows — No size grip**: No size grip visible in the lower-right corner of the status bar. Win95 sizable windows should show a size grip.
   → **Fix**: Add a diagonal hatching size grip in the lower-right corner of the status bar.

6. **Menus — Context menus**: Cannot verify right-click context menus from screenshots. Win95 requires pop-up context menus on button 2 (right-click) for all interactive areas.
   → **Fix**: Ensure right-click context menus are available in the document area, toolbar, and status bar.

### Accessibility Violations

1. **No access keys on any visible UI element**: Neither menu items nor toolbar buttons show access key underlines, blocking keyboard-only users from navigating the UI.
   → **Fix**: Implement Alt+key access for all menus, and ensure Tab/Shift+Tab navigation works across toolbar, formatting bar, and document.

2. **Color as sole cue in status bar**: Green "Auto-saved" text uses color as the only differentiation from normal status text. Users with color vision deficiency may miss this.
   → **Fix**: Add an icon or use bold text weight alongside color to indicate auto-save status.

3. **Toast notifications block UI**: Stacked toasts prevent interaction with toolbar. No keyboard shortcut to dismiss them.
   → **Fix**: Auto-dismiss toasts, limit count, and add Escape to dismiss.

### Strengths
- Title bar correctly uses COLOR_ACTIVECAPTION (0,0,128 navy blue) with white text
- Menu bar uses measured text widths, creating professional spacing
- File menu dropdown has proper ellipsis conventions ("New from Template...", "Open...")
- File menu has right-aligned keyboard shortcuts (Ctrl+N, Ctrl+O)
- Menu separator lines correctly group related items
- 3D bevel borders on toolbar buttons match Win95 raised/sunken style
- Win95 color palette is accurate (gray 192, navy, white)
- Layout order is correct: Title > Menu > Toolbar > Formatting > Ruler > Document > Status
- Status bar in migrated UI faithfully reproduces Word 6.0 layout (Page/Sec/1/1/At/Ln/Col + REC/MRK/EXT/OVR + clock)
- Selection highlight uses correct COLOR_HIGHLIGHT (0,0,128) with white text
- Formatting bar dropdowns (style, font, size) correctly positioned

### Overall Windows Interface Compliance: 4/10

The structural skeleton is solid and the color palette is faithful, but the lack of proper icons, window controls, scroll bars, access keys, and the toast notification issues significantly reduce compliance. The two inconsistent UI states (full toolbar vs. simplified) also hurt. Fix the critical items and this jumps to 6-7; fix major items too and it's an 8.

---

# audit-sun-design: Screenshot Audit (2026-02-08)

Reviewed the same 16+ screenshots. Note: This application intentionally targets Win95 style, so many JLF findings reflect fundamental style differences rather than bugs. However, applying JLF principles reveals useful cross-platform design insights.

## Audit Summary: Full Application UI (Java L&F Perspective)

### Critical Issues (Fix Immediately)

1. **Visual Identity — Beveled instead of Flush 3D**: The entire UI uses Win95-style raised/sunken beveled borders on toolbars, buttons, and containers. JLF specifies flush 3D where components appear at the same level as the canvas with subtle edge effects. The beveled style creates unnecessary visual noise.
   → **Fix**: If targeting JLF, replace all `BevelStyle::Raised/Sunken` with flush borders — thin 1px highlight on top/left, 1px shadow on bottom/right, no outer border. (N/A if staying Win95-only.)

2. **Visual Identity — No drag texture**: Toolbars lack the JLF drag texture (small dotted pattern at the left edge) that indicates the toolbar is movable/dockable. JLF toolbars must show this texture.
   → **Fix**: Add a 10px-wide textured drag handle on the left side of each toolbar using the JLF dot pattern.

3. **Visual Design — Wrong color model**: UI uses Win95 system colors (gray #C0C0C0, navy #000080, white) rather than JLF's 8-color theme model. JLF derives all colors from 3 primary colors (102-102-153, 153-153-204, 204-204-255), 3 secondary colors (102-102-102, 153-153-153, 204-204-204), plus black and white.
   → **Fix**: Implement the JLF MetalTheme color model. Backgrounds should use Secondary 3 (#CCCCCC), focus rings Primary 2 (#9999CC), selection Primary 3 (#CCCCFF).

4. **Behavior — No mnemonics on any UI element**: Menu titles and menu items lack mnemonic underlines. JLF requires mnemonics on every menu, every menu item, and every labeled control in dialogs.
   → **Fix**: Add mnemonic underlines to all menu titles and items. Assign unique mnemonics within each menu scope.

5. **Behavior — No keyboard shortcut annotations visible**: Only File menu shows Ctrl+N/Ctrl+O. JLF requires standard shortcuts (Ctrl+S, Ctrl+P, Ctrl+Z, Ctrl+X, Ctrl+C, Ctrl+V, Ctrl+F, Ctrl+A) to be visible in menus.
   → **Fix**: Show all standard keyboard shortcuts in their respective menus.

### Major Issues (Fix Before Launch)

1. **Menus — Wrong standard order for JLF**: JLF menu order is File, Object, Edit, Format, View, [app-specific], Help (always last). Current order: File, Edit, View, Format, Insert, Table, Help, Tools, Settings. Edit should precede Format; View should follow Format; Help must be last.
   → **Fix**: Reorder to JLF standard. "Settings" should be under a "Preferences" item in the File or Edit menu (JLF convention: Edit > Preferences for cross-platform, or dedicated Preferences dialog).

2. **Toolbars — No tool tips**: Toolbar buttons appear to lack tool tips. JLF requires tool tips on every toolbar button that doesn't have a text label. Since buttons use single-letter labels (ambiguous), tool tips are essential.
   → **Fix**: Add descriptive tool tips to all toolbar buttons (e.g., "New Document (Ctrl+N)"). Tool tips should appear after 700ms hover.

3. **Toolbars — Wrong button sizing and spacing**: JLF specifies 25x25 toolbar buttons, 2px spacing between individual buttons, and 11px spacing between button groups. Current toolbar appears to use custom sizes without clear group spacing.
   → **Fix**: Standardize to 25x25 buttons, 2px individual spacing, 11px group spacing.

4. **Toolbars — No mouse-over border**: JLF toolbar buttons should show a raised border only on mouse-over, appearing flat otherwise. Current buttons show permanent raised borders (Win95 style).
   → **Fix**: Implement mouse-over border reveal for toolbar buttons.

5. **Feedback — Toast notification system non-standard**: "[i] Auto-saved" toast notifications are not a JLF pattern. JLF uses status bar messages for operational feedback and standard alert dialogs for important messages. The toast stacking issue (10+ toasts) is especially problematic.
   → **Fix**: Use the status bar for auto-save notifications. Reserve dialogs/alerts for user-actionable events.

6. **Application Graphics — Text labels instead of icons**: JLF requires toolbar icons to use flush 3D rendering at 16x16 (small) or 24x24 (large) size. Single letters are not acceptable button graphics.
   → **Fix**: Create flush 3D-style icons for all toolbar actions.

7. **Visual Design — Non-6px spacing**: Inter-component spacing doesn't follow JLF's 6px base unit. Gaps between toolbar and formatting bar, between formatting bar and ruler, etc., appear to use arbitrary values.
   → **Fix**: Align all spacing to multiples of 6px (5px actual within groups for 6px perceived, 11px actual between groups for 12px perceived).

8. **Windows — Status bar "Auto-saved" in green**: Green (#00FF00 or similar) is not part of the JLF 8-color model. Status text should use black text on Secondary 3 background.
   → **Fix**: Use black text for status messages.

### Minor Issues (Nice to Have)

1. **Visual Design — Typography**: Document text uses a serif font (Times New Roman) which is appropriate for content, but UI elements (menu bar, status bar, toolbar labels) should use the JLF "Dialog" logical font (mapped to SansSerif on most platforms). Some screenshots show what appears to be a monospace font in the title bar.
   → **Fix**: Use sans-serif for all UI chrome; reserve serif for document content only.

2. **Visual Design — Capitalization**: Menu items generally use headline capitalization ("New from Template...") which is correct for JLF menus. Verify that dialog labels and status bar use sentence capitalization where appropriate.
   → **Fix**: Audit all labels for correct JLF capitalization rules (headline for menus/titles, sentence for messages/descriptions).

3. **Windows — No split pane zoom buttons**: If the app supports split view (View > Split View visible in dark mode screenshot), JLF recommends zoom buttons for split panes.
   → **Fix**: Add one-touch zoom buttons to split pane dividers.

4. **Controls — Combo box style**: Style/font/size dropdowns in the formatting bar use a custom dropdown appearance. JLF specifies specific combo box styling with flush 3D borders.
   → **Fix**: Style dropdowns as JLF noneditable combo boxes with proper borders.

5. **Application Graphics — No splash screen**: JLF recommends a splash screen for applications with startup delay. Not visible in screenshots.
   → **Fix**: Consider adding a splash screen if startup takes > 2 seconds.

### Accessibility Violations

1. **No visible mnemonics**: Complete absence of mnemonic underlines on menus and controls. JLF mandates mnemonics on all interactive elements for keyboard accessibility.
   → **Fix**: Add mnemonics following JLF conventions (first letter preferred, unique within scope).

2. **Ambiguous toolbar labels**: Single-letter button labels (N, O, S, P) provide no accessible description. Screen readers would announce "N button" which is meaningless.
   → **Fix**: Set accessible names on all toolbar buttons (e.g., "New Document", "Open File", "Save").

3. **Color-only feedback**: Green "Auto-saved" text relies on color alone. JLF requires information to be conveyed through multiple channels.
   → **Fix**: Add an icon or text prefix (e.g., "Status: Auto-saved") alongside color.

4. **Toast notifications inaccessible**: Stacked toasts have no keyboard dismiss mechanism and may not be announced by screen readers.
   → **Fix**: Make notifications accessible via live region announcements and keyboard-dismissible.

### Strengths
- Selection highlight (navy background, white text) is visually clear
- Menu separator grouping is logical and matches JLF conventions
- File menu includes appropriate ellipsis on dialog-launching items
- Keyboard shortcuts shown in menus (Ctrl+N, Ctrl+O, Ctrl+Enter, Ctrl+K)
- Document text rendering is clean and readable
- Status bar provides contextual info (Page/Sec/Ln/Col) per JLF status bar guidelines
- Title bar correctly shows document name and modification state (asterisk)
- Menu items are well-organized into logical groups

### Overall Java Look and Feel Compliance: 2/10

The application intentionally targets Win95 style, so low JLF compliance is expected. The beveled 3D, Win95 color palette, and lack of flush 3D/drag texture are fundamental mismatches. However, several JLF-relevant findings are actionable regardless of target style: missing mnemonics, missing tool tips, toast notification issues, accessibility gaps, and missing scroll bars. These cross-guideline issues should be fixed regardless of which design language is targeted.

---

# audit-design (Apple HIG 1987): Screenshot Audit (2026-02-08)

Reviewed 16+ screenshots covering: default empty state, text typing, bold/italic/underline formatting, menu dropdowns (File, View, Insert), toolbar states, multiline editing, indentation, lists, status bar, table insert, bookmark, text color, page break, double-click selection, smart quotes, dark mode toggle, and toast notification stacking.

## Audit Summary: Full Application UI (Apple HIG Perspective)

### Critical Issues (Fix Immediately)

1. **Metaphors — Toolbar buttons use letters, not real-world objects**: Toolbar buttons labeled N, O, S, P, X, C, V, <, > fail the metaphor principle. Users must *remember* what each letter means rather than *recognize* a familiar icon (folder=Open, floppy=Save, scissors=Cut). This also violates "See-and-Point" — users can't discover function by looking.
   → **Fix**: Replace letters with recognizable icons depicting real-world objects. Even simple pixel-art icons (page, folder, floppy, printer) dramatically improve recognition.

2. **User Control — Toast notifications block the interface**: Screenshots show 10+ "[i] Auto-saved" toasts stacking vertically, covering the toolbar and formatting bar. The user cannot dismiss them, cannot interact with covered controls, and cannot prevent them from appearing. This is a severe "User Control" violation — the software is in charge, not the user.
   → **Fix**: Limit toasts to 1 max. Use the status bar for auto-save feedback (non-intrusive). If toasts are kept, make them dismissible on click and auto-expire in 2-3 seconds.

3. **Feedback — Inconsistent UI states between views**: Some screenshots show the full UI (toolbar + formatting bar + ruler + status bar), while others show a stripped-down view (just menu bar + document + simple status bar). The user has no clear indication of which mode they're in or why UI elements have disappeared. This violates "Perceived Stability" — the interface changes unpredictably.
   → **Fix**: Keep the UI layout consistent. If toolbar/formatting bar can be hidden, provide clear View menu toggles and remember the user's preference.

4. **Forgiveness — No visible Undo/Redo affordance**: While Ctrl+Z may work, there's no visible Undo button in the toolbar (the < and > buttons are ambiguous). The Apple HIG principle of Forgiveness requires that recovery from mistakes be obviously accessible. Users shouldn't have to know keyboard shortcuts to undo.
   → **Fix**: Add clearly labeled Undo/Redo toolbar buttons with recognizable curved-arrow icons.

5. **See-and-Point — No scroll bar**: The document area has no visible scroll bar. Users must *remember* to use keyboard shortcuts or scroll gestures — there's no visible affordance to *point at* for scrolling. This fundamentally violates the "See-and-Point" principle.
   → **Fix**: Add a visible scroll bar. Per Apple HIG, scroll bars should always be visible and indicate content position.

### Major Issues (Fix Before Launch)

1. **Consistency — Menu order differs from convention**: Menu order varies between screenshots: some show "File, Edit, View, Format, Insert, Table, Help, Tools, Settings" and others show "File, Edit, View, Format, Insert, Table, Help, Tools" (no Settings). Internally inconsistent, and neither matches the Apple standard (File, Edit, View, special menus..., Help/Apple menu).
   → **Fix**: Standardize menu order. Remove "Settings" (merge into existing menu). Help should be last.

2. **Consistency — Two different status bar designs**: The migrated UI shows a Word-6.0-style status bar (Page/Sec/Ln/Col + indicators + clock). Older screenshots show a simple gray bar with green "Auto-saved" text. These are completely different designs for the same component.
   → **Fix**: Use one consistent status bar design everywhere.

3. **Direct Manipulation — No visible drag handles or resize affordances**: Toolbars have no drag handles to indicate they can be rearranged. The ruler lacks draggable margin/indent handles. No size grip on the window. Users can't discover these direct manipulation opportunities.
   → **Fix**: Add visual drag handles and grips where direct manipulation is possible.

4. **Feedback — Formatting dropdown "v" gives no feedback on state**: The formatting bar dropdowns show "Normal v", "Times New Roman v", "10 v" with a lowercase "v" as the dropdown indicator. This doesn't clearly communicate that these are interactive controls. Proper affordance requires a standard dropdown arrow or frame.
   → **Fix**: Use a proper dropdown triangle glyph and add a visible frame/border to indicate interactivity.

5. **WYSIWYG — Selection highlight obscures text**: In multiple screenshots (bold, underline, text color), the selection highlight (deep navy #000080 background) almost completely obscures the selected text, making it unreadable. WYSIWYG demands that users always see their content clearly.
   → **Fix**: Ensure selection highlight maintains readable contrast. Consider a lighter selection color or ensure text is drawn in white/bright over the highlight.

6. **Modes — No clear mode indicators for formatting**: When bold is active, the B button appears slightly darker but the change is subtle. When underline is active, the U button is slightly brighter but the mode isn't obvious. Apple HIG warns that modes are dangerous because users forget they're in one.
   → **Fix**: Make active formatting modes visually prominent — clearly sunken button, changed color, or checkmark indicator.

7. **Feedback — Insert menu items lack state feedback**: The Insert menu shows many items (Page Break, Section Break, Hyperlink, Table, Image, shapes, etc.) but none show whether they're currently applicable or available. Disabled items should be grayed out to provide feedback.
   → **Fix**: Gray out items that cannot be applied in the current context (e.g., "Remove Hyperlink" when cursor isn't on a hyperlink).

### Minor Issues (Consider Fixing)

1. **Aesthetic Integrity — Visual noise from beveled borders**: The heavy 3D bevel borders on every toolbar button, container, and bar create visual clutter. Apple HIG's "Aesthetic Integrity" principle values clarity — visual design should serve the content, not compete with it.
   → **Fix**: Consider reducing border weight or using flatter styling for less-important containers. (Note: This may conflict with the Win95 design target.)

2. **Plain Language — Status bar abbreviations**: "REC", "MRK", "EXT", "OVR" are cryptic abbreviations that require memorization. Apple HIG prefers plain language that users understand immediately.
   → **Fix**: Consider tooltips on hover that explain each abbreviation (e.g., "REC = Macro Recording").

3. **Perceived Stability — Title bar asterisk convention**: Some screenshots show "Wordproc - Untitled *" (with asterisk for unsaved changes) while others show "Wordproc - Untitled" (no asterisk). The asterisk is a subtle, easy-to-miss change indicator.
   → **Fix**: Consider a more visible unsaved-changes indicator (e.g., dot in close button, or explicit "[Modified]" text).

4. **Consistency — Smart quotes rendering**: In `e2e_smart_quotes.png`, the text shows "\?Hello\?" with backslashes, suggesting smart quote rendering issues. The display should match what the user typed (WYSIWYG).
   → **Fix**: Debug smart quote rendering to show proper curly quotes without escape characters.

5. **See-and-Point — No visible keyboard shortcuts on most menus**: Only File menu shows shortcuts (Ctrl+N, Ctrl+O). Users looking at Edit, View, Format menus can't discover shortcuts by scanning.
   → **Fix**: Show keyboard shortcuts for all common operations in all menus.

### Strengths
- Clean document editing area with good whitespace
- Proper use of the selection model (select first, then act) — "Hey you, do this" paradigm
- Menu separators correctly group related commands
- Ellipsis convention used properly on dialog-launching items (New from Template..., Open..., Hyperlink..., Bookmark...)
- File menu has proper shortcut annotations (Ctrl+N, Ctrl+O)
- Title bar shows document name — user always knows what they're editing
- Double-click word selection works (visible in e2e_double_click_word.png)
- Multiple text formatting options visible and discoverable in menus
- Dark Mode option available in View menu — respects user preference
- Ruler provides spatial reference for document layout

### Overall Apple HIG Compliance: 4/10

The application gets the fundamentals right — it uses menus, has a clear document metaphor, supports select-then-act, and provides formatting options. But the severe toast notification issue, lack of recognizable icons, missing scroll bar, and inconsistent UI states across different views significantly undermine usability. The WYSIWYG principle is weakened by the opaque selection highlight. Fix the critical items (icons, toasts, scroll bar) and address the consistency issues to reach 6-7.

---

# audit-google-design (Material Design 3): Screenshot Audit (2026-02-08)

Reviewed 16+ screenshots. Note: This application is a Win95-styled desktop word processor, so it fundamentally targets a different design language than Material Design 3. However, M3 principles around accessibility, feedback, and interaction are universal and provide useful cross-cutting insights.

## Audit Summary: Full Application UI (M3 Perspective)

### Critical Issues (Fix Immediately)

1. **Touch/Interaction — Touch targets far below 48dp**: Toolbar buttons appear to be approximately 20-24px squares. Even accounting for desktop use (44dp pointer target minimum), these are significantly undersized. The formatting bar buttons (B, I, U, L, C, R, J) are similarly small. This makes precise interaction difficult even with a mouse.
   → **Fix**: Increase toolbar button sizes to at least 32x30px (Win95 large toolbar) or preferably 36x36px minimum for comfortable clicking.

2. **Accessibility — No focus indicators visible**: None of the screenshots show keyboard focus indicators on any UI element — no focus ring on toolbar buttons, menu items, dropdowns, or document area. Keyboard users cannot tell where focus is.
   → **Fix**: Add visible focus indicators (2px outline, 3:1 contrast minimum against adjacent colors) to all interactive elements. Use the Win95 dotted focus rectangle convention.

3. **Accessibility — Color-only meaning**: The status bar "Auto-saved" text is displayed in green — the only visual distinction from normal text is color. This excludes users with red-green color vision deficiency. Additionally, selection highlight uses color alone (navy blue) with no other visual cue.
   → **Fix**: Supplement color with additional cues: an icon prefix for status messages, and ensure selection highlight maintains text legibility.

4. **Color System — No semantic color roles**: The UI uses a fixed palette (gray #C0C0C0, navy #000080, white #FFFFFF, green for status). M3 defines 26 semantic color roles (primary, on-primary, primary-container, secondary, tertiary, error, surface, etc.) that adapt across themes. The Win95 palette has no error color, no warning color, no surface hierarchy.
   → **Fix**: Even within a Win95 aesthetic, define semantic roles: error state (red), warning state (yellow), success state (green), disabled state (gray text), to ensure consistent meaning across the app.

5. **States — Only 2-3 of 7 states distinguishable**: M3 defines 7 interactive states: enabled, disabled, hovered, focused, pressed, dragged, selected. From the screenshots, only enabled, pressed (partially), and selected (text selection) are visually distinguishable. Hover, focus, disabled, and dragged states are not visible.
   → **Fix**: Implement distinct visual treatment for all interactive states, especially hover (slight highlight), focus (dotted rectangle), and disabled (grayed out text + no bevel).

### Major Issues (Fix Before Launch)

1. **Typography — No type hierarchy**: M3 uses 5 type roles (Display, Headline, Title, Body, Label) with clear size/weight progression. The current UI uses roughly 2 sizes: small UI text (menus, status bar, toolbar) and document text. There's no clear hierarchy between the title bar, menu bar, toolbar labels, formatting labels, and status bar text.
   → **Fix**: Define at least 3 distinct type styles: Title (title bar), UI Label (menus, toolbar), Body (document), Status (status bar). Use consistent sizes and weights.

2. **Elevation — No elevation hierarchy**: M3 uses 6 elevation levels to communicate stacking order and importance. The current UI uses 3D borders but doesn't create a clear depth hierarchy. Menu dropdowns, toolbars, and the document area all appear at similar visual depths despite different functional layers.
   → **Fix**: Ensure dropdown menus have a visible drop shadow to float above the document. The toolbar should feel slightly above the document. The status bar should feel grounded at the bottom.

3. **Components — Toast/Snackbar implementation violates M3**: M3 Snackbars are single-line, appear at the bottom of the screen, show one at a time, auto-dismiss after 4-10 seconds, and include an optional action button. The current "[i] Auto-saved" toasts appear at the top-center, stack unlimitedly, have no action button, and no dismiss mechanism.
   → **Fix**: If implementing a snackbar-like pattern, follow M3: bottom-positioned, single instance, auto-dismiss, with optional "Dismiss" action. Better yet, use the status bar.

4. **Layout — No responsive adaptation**: M3 defines 5 window size classes (Compact, Medium, Expanded, Large, Extra-Large) with different layouts. The application appears to be fixed-size with no adaptation. When the window is smaller (visible in some older screenshots), the toolbar labels get truncated ("Tool" instead of "Tools") rather than adapting gracefully.
   → **Fix**: Implement responsive behavior: hide toolbar labels at narrow widths, collapse formatting bar into an overflow menu, ensure menus don't clip at the screen edge.

5. **Shape — No shape system**: M3 uses a 10-level corner radius scale. The current UI uses exclusively 0px corner radii (sharp corners) on every element. While this is appropriate for Win95, it means there's no shape-based differentiation between element types.
   → **Fix**: N/A if targeting Win95. But consider: even Win95 used subtle shape differences (rounded buttons in some dialogs, rounded scroll bar thumbs).

6. **Motion — No motion/animation**: M3 emphasizes physics-based spring animations for state transitions. The current UI has no visible motion — menus appear/disappear instantly, toasts appear without animation, toolbar states change without transition.
   → **Fix**: Consider subtle open/close animation for menu dropdowns (fast, 100-200ms). Toast slide-in/fade-out. Button press feedback animation.

7. **Dark Theme — Incomplete implementation**: The View menu shows a "Dark Mode" option, but no screenshot shows the dark mode actually active. If dark mode exists, M3 requires: avoid pure black (#000000) surfaces, use tonal elevation, maintain contrast ratios, and adapt all semantic colors.
   → **Fix**: Verify dark mode uses dark gray surfaces (not pure black), maintains 4.5:1 text contrast, and adapts all UI colors (not just the document area).

8. **Components — Button hierarchy unclear**: M3 requires clear button hierarchy — one primary action should stand out. The toolbar shows 9+ buttons all with identical visual treatment (raised, gray, same size). There's no visual hierarchy indicating which buttons are most important.
   → **Fix**: Even in Win95 style, consider visual grouping: primary actions (New, Open, Save) in one cluster, clipboard actions (Cut, Copy, Paste) in another, with separators. Size or position the most-used buttons to stand out.

### Minor Issues (Nice to Have)

1. **Iconography — Text labels instead of icons, no style consistency**: M3 requires consistent icon style (outlined, rounded, or sharp — pick one). Current toolbar uses single letters which are not icons at all.
   → **Fix**: When adding icons, choose one consistent style and stick with it throughout.

2. **Writing/Voice — Status bar abbreviations**: "REC", "MRK", "EXT", "OVR" are technical abbreviations. M3 writing guidelines favor clear, concise language that users understand without training.
   → **Fix**: Add tooltips explaining each abbreviation.

3. **Components — Menu has no icons**: M3 menus can include leading icons for quick recognition. Current menus are text-only.
   → **Fix**: Consider adding small icons to frequently-used menu items (New, Open, Save, Print, Cut, Copy, Paste, Undo, Redo).

4. **Spacing — Inconsistent gaps**: The spacing between title bar, menu bar, toolbar, formatting bar, and ruler varies. M3 uses an 8dp grid for consistent spacing.
   → **Fix**: Normalize all inter-component spacing to consistent values (even if not 8dp, at least make it uniform).

5. **Components — No progress indicators**: No loading state or progress indicator visible in any screenshot. If operations (save, print, export) take time, M3 requires progress feedback.
   → **Fix**: Add progress indicators for operations > 1 second.

### Accessibility Violations

1. **[WCAG 1.4.1 — Use of Color]**: Green "Auto-saved" text uses color as the sole differentiator. The navy selection highlight may not provide 4.5:1 contrast against dark text.
   → **Fix**: Add non-color cues (icon, font weight) for status. Verify selection highlight contrast.

2. **[WCAG 2.1.1 — Keyboard]**: No evidence of keyboard focus navigation in any screenshot. If Tab/Shift-Tab navigation between toolbar, formatting bar, and document area isn't implemented, keyboard-only users are blocked.
   → **Fix**: Implement full Tab navigation with visible focus indicators.

3. **[WCAG 2.4.7 — Focus Visible]**: No focus indicators visible on any interactive element in any screenshot.
   → **Fix**: Add dotted focus rectangles (Win95 convention) or 2px solid outlines on focused elements.

4. **[WCAG 1.4.3 — Contrast]**: Title bar text (white on navy #000080) appears readable but should be verified. Menu text (dark on gray #C0C0C0) appears adequate. Status bar green text on gray background may not meet 4.5:1.
   → **Fix**: Verify all text contrast ratios with a tool. Green (#00xx00 variants) on gray (#C0C0C0) often fails 4.5:1.

5. **[WCAG 4.1.2 — Name, Role, Value]**: Single-letter toolbar buttons (N, O, S) provide no accessible name. Screen readers would announce meaningless labels.
   → **Fix**: Add accessible names/labels to all toolbar buttons describing their function.

### Strengths
- Clear document-centric layout with obvious content area
- Menu bar provides discoverability — all features accessible through menus
- Keyboard shortcuts shown in File menu (Ctrl+N, Ctrl+O)
- Proper ellipsis convention on dialog-launching menu items
- Logical menu grouping with separators
- Selection highlight color provides clear visual feedback for selected text
- Status bar provides contextual information (page, section, line, column)
- Title bar shows document name and modification state
- Multiple zoom levels available (View > Zoom In/Out/Reset)
- Split View option available for side-by-side editing

### Overall Material Design 3 Compliance: 1/10

Expected for a Win95-targeted desktop application — M3 is designed for modern mobile/web surfaces. However, the cross-cutting takeaways are valuable: accessibility is weak (no focus indicators, color-only cues, no accessible names), interactive states are incomplete (only 2-3 of 7), and the toast notification pattern is broken across all guidelines. The most actionable M3 insights for this app: implement all 7 interactive states, add focus indicators, add accessible names, fix the toast/snackbar pattern, and verify contrast ratios.

---

# Re-Audit: 2026-02-08 (Post Implementation Pass 1)

Screenshots reviewed: `audit_01_default_view.png` (empty document), `audit_02_with_text.png` (with text typed, showing [Modified]).

## Items Addressed (Verified in Screenshots)

| # | Item | Status | Evidence |
|---|------|--------|----------|
| 3 | Toast notification stacking | ✅ FIXED | `dismiss_all()` clears existing toasts before new ones. Auto-save disabled in test mode. No stacking visible. |
| 5 | Menu order + Help last | ✅ FIXED | Menu bar shows: File, Edit, View, Insert, Format, Tools, Table, Help. Settings merged into Tools. Help is last. |
| 7 | Formatting button active state | ✅ CONFIRMED WORKING | `absToolbarButton()` uses `BevelStyle::Sunken` + `TOOLBAR_PRESSED_BG` when `pressed=true`. Alignment buttons show sunken state for active alignment. |
| 9 | Status bar green text → neutral | ✅ FIXED | Toast messages now use text prefixes: `[saved]`, `[error]`, `[recovered]` instead of color-only. |
| 11 | Color-only status feedback | ✅ FIXED | Text prefixes added: `[saved] Auto-saved`, `[error] Auto-save failed`, `[recovered] Restored from auto-save`. |
| 13 | Window control buttons | ✅ FIXED | Min (\_), Max (o), Close (X) buttons visible on right side of title bar with Win95-style beveled borders. |
| 15 | Toolbar separators etched | ✅ FIXED | `drawEtchedSeparator()` renders dark+light line pairs between button groups. Visible in screenshots between N/O/S and P, between P and X/C/V, between V and </>, and in formatting bar. |
| 27 | Title bar unsaved indicator | ✅ FIXED | Title shows "Wordproc - Untitled [Modified]" after typing (visible in `audit_02_with_text.png`). Much clearer than the old asterisk. |

## Items Still Outstanding (Visible in Screenshots)

### Critical (from all 4 audits)
| # | Item | Status | Notes |
|---|------|--------|-------|
| 1 | Toolbar icons | ❌ STILL TODO | Buttons still show single letters: N, O, S, P, X, C, V, <, > |
| 2 | Vertical scroll bar | ❌ STILL TODO | No scroll bar visible on document area |
| 4 | Tooltips on toolbar buttons | ❌ STILL TODO | Cannot verify from screenshots but code has no tooltip implementation |

### Major
| # | Item | Status | Notes |
|---|------|--------|-------|
| 6 | Access keys / mnemonics | ❌ STILL TODO | No underlined letters visible on menu titles or items |
| 10 | Keyboard shortcuts on all menus | ❌ STILL TODO | Only File menu shows shortcuts |
| 12 | Focus indicators | ❌ STILL TODO | No keyboard focus rectangles visible |
| 14 | Text area sunken border | ❌ STILL TODO | Document area has no sunken bevel border |
| 17 | Title bar font | ❌ STILL TODO | Title bar text still appears monospace-like |
| 18 | Selection highlight contrast | ❓ UNTESTED | No selection visible in current screenshots |
| 19 | Insert menu too long | ❌ STILL TODO | Shapes not yet in cascading submenu |
| 20 | Hover state on buttons | ❌ STILL TODO | No hover state visible |
| 22 | Consistent spacing | ❌ STILL TODO | Gaps between UI bands still appear slightly uneven |

### Minor
| # | Item | Status | Notes |
|---|------|--------|-------|
| 8 | Dropdown arrow glyph | ❌ STILL TODO | Still shows "v" instead of ▼ |
| 16 | Status bar consistency | ❓ UNTESTED | Status bar looks consistent in current screenshots |
| 21 | Status bar abbreviation tooltips | ❌ STILL TODO | REC/MRK/EXT/OVR still unexplained |
| 24 | Context menus | ❌ STILL TODO | Cannot verify from screenshots |
| 25 | Insert menu disabled states | ❌ STILL TODO | No grayed-out items visible |

## Updated Scores (after phase 3)

| Audit | Original | After Phase 2 | After Phase 3 | Change |
|-------|----------|---------------|---------------|--------|
| Win95 | 4/10 | 5.5/10 | 6.5/10 | +2.5 — Scroll bar, tooltips, insert menu cleanup, all phase 2 items |
| Apple HIG | 4/10 | 5/10 | 6/10 | +2.0 — Scroll bar (See-and-Point), tooltips (Forgiveness), insert menu |
| Sun JLF | 2/10 | 2.5/10 | 3/10 | +1.0 — Tool tips added, scroll bar |
| Material Design 3 | 1/10 | 1.5/10 | 2/10 | +1.0 — Scroll bar, tooltips |

## Summary (Phase 3)

12 of 38 "Yes" items addressed across all phases:

**Done:** #3 (toast stacking), #5 (menu order), #7 (formatting active state), #9 (status bar color), #11 (color-only feedback), #13 (window controls), #14 (sunken border - already existed), #15 (etched separators), #19 (insert menu consolidated), #27 (title bar [Modified])
**Done (new):** #2 (scroll bar), #4 (tooltips)

**Deferred to afterhours** (see `docs/plans/afterhours-feature-requests.md`):
- ~~#1 (toolbar icons)~~ — WORKAROUND: pixel-art icons drawn with raylib primitives in ToolbarOverlayRenderSystem
- ~~#6 (access keys/mnemonics)~~ — WORKAROUND: underlines drawn via raylib DrawLine in overlay render
- ~~#8 (dropdown ▼)~~ — WORKAROUND: filled triangles drawn via raylib DrawTriangle in overlay render
- #12 (focus indicators) — needs visible focus ring rendering from ComputeVisualFocusId
- ~~#17 (title bar font)~~ — RESOLVED: loaded Roboto-Regular.ttf as UI font
- ~~#20 (hover state)~~ — WORKAROUND: was_hot() used for flat/raised/sunken toolbar button states
- #34 (drop shadows on menus) — needs shadow/elevation support in afterhours
- #36 (motion/animation) — needs transition system for opacity/position interpolation

**Remaining local work:**
- #10 (keyboard shortcuts on all menus) — show shortcuts beyond just File menu
- #16 (status bar consistency), #18 (selection contrast), #21 (status bar abbreviation tooltips)
- #22 (consistent spacing), #24 (context menus), #25 (disabled menu items)
- #28 (ruler alignment), #29 (ruler margin handles), #33 (type hierarchy)
- #35 (responsive layout), #38 (progress indicators)
