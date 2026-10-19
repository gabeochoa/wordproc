# Word for Windows 95 (Word 7.0) — Parity Checklist

Status key: **Have** = works end-to-end (menu/toolbar + behavior + test) ·
**Partial** = present but incomplete vs Word 95 · **Missing** = absent.
Priority: P0 = daily-driver value, P1 = common, P2 = niche/stub-OK.
Inventory by code read, 2026-10-18 (`src/ui/menu_setup.h`, `src/ecs/menu_actions.h`,
`src/editor/*`, `tests/`). Counts at bottom are computed from these tables.

## File
| Feature | Status | Priority | Notes |
|---|---|---|---|
| New / Open / Save / Save As | Have | P0 | native dialogs, recent files |
| New from Template... | Have | P1 | Win95 dialog lists the shipped .wpdoc templates in resources/templates/ (Letter/Memo/Report, scanned via the afterhours files resource path); OK opens the selected template as a NEW Untitled document (filePath cleared, dirty=true, undo history cleared). Templates carry Title/Heading 1 styles via the .wpdoc `paragraphStyles` array (see docs/file_format.md); the legacy .txt templates are gone |
| Export RTF / HTML / PDF | Have | P1 | File menu exports; native .doc binary I/O Missing below |
| Save/open .doc (Word binary) | Missing | P2 | RTF is the interchange path |
| Save/open .rtf / .txt | Have | P0 | document_io + tests |
| Page Setup (margins/paper) | Have | P0 | File > Page Setup... Win95 dialog: paper presets (Letter/Legal/Tabloid/A4/A5/B5/Executive/Custom), Portrait/Landscape, four margin fields in inches. OK writes the whole block to `DocumentSettings.pageSettings` and syncs the layout (`cmd::applyPageSettings`), so the paged renderer, ruler white span, hit-test, and Print Preview page-count estimate all consume the same per-side margins + paper dims (Letter landscape = 11 x 8.5in, 0.5in margins = 36pt). Size/orientation/per-side margins persist in `.wpdoc` `pageLayout`; legacy files without per-side keys fall back to `pageMargin`. View > Normal/Page Layout persists to settings (preview/outline switches stay transient). Unit + E2E `pass_page_setup_dialog.e2e` |
| Print | Have | P1 | File > Print... / Ctrl+P / toolbar Print: PDF via export_pdf to a temp file, then `lp`; in test mode the PDF is written but `lp` never runs. Honest partial: export_pdf is single-page with overflow past the bottom margin, so Print prints what's supported, not a true multi-page job |
| Print Preview | Have | P1 | Batch 3 + fix round: read-only mode (File > Print Preview, toolbar button); forces Paged, one page on gray workspace. Printable content ONLY: no spell squiggles, caret, selection, page-break indicator, or line numbers in preview (gated in the render path; paged editing view unchanged). Preview toolbar is ONE compact ~28px row of beveled Win95 buttons: [Print icon] [Prev] [Next] [Page X of Y] [Zoom dropdown 50/75/100/150/200%] [Close] — the old bar was double height (both toolbars). Ruler hidden in preview (Word default; showRuler flag untouched, restored on exit) and Formatting bar gone, so chrome above the text shrinks (text_area_y 153 -> 96 at ui_scale 1.2). Status bar shows "Page X of Y" + zoom % instead of Ln/Col/At and the REC/MRK/EXT/OVR block. Esc/Close restores the previous mode; typing/scroll gated; zoom recomputes the pagination estimate. Page count is an ESTIMATE from geometry (page text height / scaled line height) — no real pagination exists, so headings/spacing shift true breaks. Fix round 2: the displayed Zoom + status % is the EFFECTIVE zoom (fit * user): entering at fit shows the fit percent, choosing effective 100% renders the page at 72ppi (page 612 wide, scrollable via existing paging); preview zoom is preview-local (pre-preview user zoom restored on exit). File header stays flat in preview: entering preview clears open menus / activeMenuIndex, and menu/dropdown close clears the stale focus id that used to paint a pressed focus border on File via a recycled entity id |
| Exit / MRU list | Have | P2 | — |

## Edit
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Undo / Redo | Have | P0 | CommandHistory, multi-level |
| Cut / Copy / Paste | Have | P0 | incl. toolbar + context menu |
| Select All | Have | P0 | |
| Find / Replace / Find Next-Prev | Have | P0 | dialog + F3/Shift+F3 |
| Go To / Bookmarks | Partial | P1 | bookmark dialog + Go To Bookmark |
| Track Changes (revise mode) | Partial | P1 | Batch 5: toggle + Accept/Reject All + per-revision Accept/Reject in Tools > Revisions...; typed inserts record Insert revisions and render green-underlined while matching; deletes are records only (see Docs-era row). Reject All reverse-applies through the command layer (one undo step per revision), replacing the old history-less baseline restore |
| AutoCorrect | Have | P0 | replace-as-type + TWo-caps + sentence cap; Tools dialog (Batch 1) |
| Repeat (F4) | Partial | P2 | Batch 7: Edit > Repeat (F4, end of Edit menu, static label). Closure-free ActionRecord (enum + payload) records the last repeatable action: typing runs (coalesced while the caret stays at the run's end), paste, formatting applications (bold/italic/underline/strike/super/sub, style, size, font, text/highlight color, alignment, indent, list, spacing) — formatting captures the resulting value, so re-applying is idempotent. Backspace/Delete/Cut clear the record; Undo/Redo leave it. Empty history = no-op + "Nothing to repeat" status flash. Ctrl+Y stays Redo (now bound on the macOS preset too, where the menu labeled it Redo but only Cmd+Shift+Z was bound). Not covered: Paragraph/Font dialog applications, Format Painter, table ops |

## View
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Normal view | Have | P1 | Batch 6 item 4: View > Normal is the Word 95 default view — checkable, first in View (Normal, Page Layout, Outline). Normal IS the old Pageless view, renamed (label only; `PageMode::Pageless` in the file format is unchanged): pageless continuous text across the full text-area width, ruler stays, plus a dashed indicator line at each computed page boundary (every lines-per-page-th row, lines-per-page = page text height / line height, the same estimate Print Preview uses — headings/spacing shift true breaks, so the lines are indicators). The renderer draws the line with no vertical shift, so hit-test math is untouched. The line sits at the new page's top edge minus half the inter-line gap (midway between the previous row's painted bottom — glyphs + squiggle — and the new row's top), never overlapping either row's glyphs. Entering flashes "Normal view" in the status bar, not a toast. Unit + E2E `pass_normal_view.e2e` (70 lines -> 2 breaks, Page Layout clears Normal, back restores it, screenshot shows the dashed line) |
| Page Layout view | Have | P0 | View > Page Layout (the old Paged Mode, renamed; dispatch is by index so no case numbers changed). Framing verified headless screenshot 2026-10-18 (white Letter page, gray workspace, text inside 1in margins). ONE scale (Batch 2 fix round): page width = 8.5 × ruler ppi, ppi = 72 × (fit × zoom), margins/page/ruler/indents all derive from it; mode switch flashes transient status-bar text ("Page Layout"), not a toast |
| Outline view | Have | P1 | View > Outline (checkable): renderer shows ONLY Title/Subtitle/Heading 1-6 paragraphs, indented (level-1)*24px; body paragraphs hidden; scroll counts visible heading rows; click maps a visible row to the heading's real row; entering forces Pageless, exiting restores the previous page mode + scroll; choosing Pageless/Paged exits Outline (scroll restored). Honest partials: arrow-key navigation can still move the caret onto a hidden body row (caret hidden until it returns to a heading); the click row estimate uses the global line height, so deep outlines with tall headings may map a click to a nearby heading (clamped, never a body row) |
| Zoom (in/out/reset, %) | Have | P0 | Ctrl+Alt +/-/0 |
| Ruler (indents/tabs draggable) | Have | P0 | Word 95 ruler: gray outside margins, white between (pageless too — 1in margins inset the effective area), inch ticks at the ONE display ppi, indent markers beveled (pale face, dark edges, hourglass pair + rectangle base + right triangle, apexes toward centre) at the same scale as text indents; left/first-line/right markers drag to set indents (one undo step per drag); View > Ruler toggles; tab-stop marks at default stops (not draggable) |
| Toolbars show/hide | Partial | P2 | Batch 7: checkable View > Standard Toolbar / Formatting Toolbar (both start shown). A hidden bar reclaims its scaled height in updateLayout (one at a time and both); the Formatting bar slides up under the menu bar when Standard is hidden; the Outlining bar occupies the Formatting slot, so hiding Formatting hides it too. Still no other Word 95 toolbars, no drag-to-reorder, no floating toolbars |
| Status bar | Have | P0 | Word 6-style; Page is computed from the scroll offset (first visible line / effective lines-per-page, clamped to the page count — the same lines-per-page estimate Print Preview and the Normal-view break lines use, times the column count in the paged columns view), Sec stays 1 (single section), Ln/Col real. The middle field is a Word 95 page fraction "Page <cur>   Sec 1   <curPage>/<totalPages>" (totalPages from the same page math, via previewPageCount), not the old row/total-lines fraction. Transient StatusComponent messages (e.g. "Normal view" / "Page Layout" after a View switch) replace the stats line for 3s — no floating toast for mode switches |
| Header/Footer view | Have | P1 | Batch 9: real headers/footers — `DocumentComponent` carries `headerBuffer` / `footerBuffer` (same `TextBuffer` as the body); View > Header and Footer (Word 95 location, after Ruler) and Insert > Header / Footer enter header/footer editing (forces Paged, restores the prior mode on Esc / Close / Insert > Document Body); a Header/Footer strip replaces the Formatting bar (target label, Page Number toggle, Switch Header/Footer, Close); typing/arrows/clipboard/Undo route to the active buffer, the body is not editable, Standard-toolbar body buttons disable; header/footer text renders wrapped in the top/bottom margin of every page in Page Layout + Print Preview (not in Normal — Word Normal hides them), `differentFirstPage` skips page 1; the old `[HEADER]` / `[FOOTER]` body-text placeholders are gone. Persisted in `.wpdoc` as `headerText` / `footerText` + `headerFooter` config (`docs/file_format.md`). Unit serialization round-trip + E2E `pass_header_footer_editing.e2e` / `pass_header_footer_save.e2e` |

## Insert
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Page/Section Break | Have | P0 | |
| Table... | Have | P0 | grid insert + Table menu |
| Picture... | Have | P1 | image insert + layout/wrap |
| Symbol / Special Character... | Partial | P1 | dialog exists; date/time insert is now Date & Time... above |
| Date & Time... | Have | P1 | Insert menu (before Special Character...): Win95 dialog, sunken list of 6 formats computed from current local time (M/d/yyyy, Month D, YYYY, Mon D, YYYY, h:MM AM/PM, D Month YYYY, ISO), Update automatically checkbox, OK inserts at caret through the command layer as one undo step (unit + E2E). Honest partial: no field engine exists, so Update automatically inserts the same static text |
| Header / Footer / Page Number | Have | P0 | Batch 9: strip Page Number button toggles the active target's center page-number field, rendered centered as "Page N" (real per-page index, `HeaderFooter::formatPageNumber`) after the footer/header text; Insert > Page Number in the body still inserts the `[PAGE #]` placeholder (no field engine) |
| Footnote | Have | P2 | |
| Hyperlink / Bookmark / Comment | Have | P1 | comment is also a Docs-era feature, kept |
| Table of Contents | Partial | P2 | entry exists; field update semantics TBD |
| Equation / Shapes | Have | P2 | beyond Word 95 basic drawing |

## Format
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Font... dialog (family/size/effects) | Have | P0 | FIRST Format item: Win95 dialog with Font list (toolbar fonts vector), Font style list (Regular/Bold/Italic/Bold Italic), Size list (16 sizes + custom field), Effects (Underline/Strikethrough/Superscript/Subscript, super/sub mutually exclusive), color swatch row (existing text colors), Preview "AaBbYyZz 123" in the chosen size/bold/color. OK applies via buffer.setTextStyle, the toolbar path (buffer style is document-global). Honest partial: Times New Roman/Arial/Courier New are toolbar names with no loaded face, so the preview family falls back to the default face (size/bold/color exact); italic has no slanted UI face in the preview |
| Bold/Italic/Underline/Strike/Super/Sub | Have | P0 | |
| Alignment L/C/R/Justify | Have | P0 | |
| Indents & Tabs (ruler + dialog) | Have | P0 | ruler drag sets indents (one undo step) + indent keys + Tab Width dialog + full Paragraph dialog (indents/spacing/line spacing, Default tab stops = document-wide tabWidth, Tabs... opens Tab Width) |
| Bullets & Numbering | Have | P0 | multi-level list indent |
| Paragraph... (spacing/line spacing) | Have | P1 | Format menu, first item of the alignment/spacing section: Win95 tabbed dialog — Tab 1 "Indents and Spacing" (Alignment dropdown Left/Centered/Right/Justified, Left/Right indents in inches, Special None/First line/Hanging + By amount, Spacing Before/After in pt, Line spacing dropdown Single/1.5 lines/Double, Tabs... button opening the Tab Width dialog; no Default-tab-stops field, per-paragraph tab stops do not exist) and Tab 2 "Line and Page Breaks" ("Page break before" starts the paragraph on a new page in Page Layout / dashed break line in Normal view, "Keep with next" moves a last-on-page paragraph with the next one; both stored per-paragraph, distinct from the Ctrl+Enter manual break). OK applies to the caret paragraph or every paragraph in the selection as one undo step through the command layer (per-row next = old, so OK never wipes borders/shading); invalid/cleared fields keep their seeded values. Unit + E2E `pass_paragraph_dialog.e2e` (indent 0.5 + after 12 + Double + Centered + Hanging 0.25, one Ctrl+Z restores all) + `pass_paragraph_dialog_pagebreak.e2e` (Tab 2 checkboxes, one Ctrl+Z restores; Tabs... opens Tab Width) |
| Borders and Shading... | Have | P1 | Format menu (before Columns...): Win95 dialog with Setting (None / Box / Custom), per-edge toggles (Top/Bottom/Left/Right), Style (None / Single line), shading swatches (None + highlight palette). OK applies border edges bitmask + style + shading index to the caret paragraph or every paragraph in the selection as one undo step through the command layer (ParagraphFormat, stored like indents; preserves each paragraph's indents/spacing). Render: shading fill behind text before selection/squiggles, border via the shared table-border drawBorderEdge helper; the border/shading rect is the paragraph line box expanded 7px horizontal / 5px vertical (4/2 base + 3 expansion) so glyphs never touch the stroke; consecutive identical paragraphs merge into one box (no internal horizontal edge). Unit + E2E `pass_borders_shading_dialog.e2e` (Box + Yellow, neighbor plain, one Ctrl+Z restores) |
| Columns... | Have | P1 | Format menu (last item): Win95 dialog with 1/2/3/4 presets, Equal column width fixed on, spacing field in inches (default 0.5, saved/loaded with the document in `pageLayout.columns` / `columnSpacing`). Paged renderer flows the page's lines through N columns (column width = (textWidth - (N-1)*spacing)/N, column 1 fills linesPerColumn top-to-bottom, then column 2...); caret rendering, click hit-test, and squiggles use the same column offsets (unit-tested math + E2E clicks in column 1/2). Honest partials: scrolling in columns is page-at-a-time (the page containing the scroll offset); Print Preview ignores columns (its own line stride); pageless is unchanged |
| Styles (Normal/Title/Headings 1-6) | Have | P0 | style dropdown + Ctrl+Alt+n |
| Drop Cap | Partial | P2 | menu entry exists |

## Tools
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Word Count... | Have | P0 | dialog: words/chars/lines/paragraphs/sentences (verified in menu_ui_system.h) |
| Spelling (F7 dialog) | Have | P0 | Batch 1: dialog + suggestions + Change All |
| Red squiggle spell check as you type | Have | P0 | Batch 1: visible-lines cache, reviewer 8/10 |
| Grammar check | Have | P2 | Batch 3 + fix round: Tools > Grammar... is a real checker mirroring the Spelling dialog — one issue at a time (sunken In context: sentence with the offending phrase; Issue: human wording, never the enum name, e.g. "Repeated word"; Suggestion: the suggestion), right column Ignore Once (advance), Ignore All (session-ignore that rule+phrase), Change (applies the suggestion through the command layer as one undo step; disabled with no suggestion), Close. Pinned: "The the cat sat." Change -> "The cat sat." (unit + E2E). Grammar issues have NO underlines: GrammarChecker has no render path — the old screenshot's wave under "sat." was a spell squiggle ("sat" was missing from the dictionary; added in fix round 2's top-1000 g–z delta). Engine covers repeated word, capitalization, double space, common errors. Fix round 2: the In-context offending span is navy-bg/white-text (sentence-relative offset/length, unit + E2E `grammar_context_span`); dialog height fits content (215 issue / 120 complete, was a fixed 300 with ~95px empty bottom) |
| AutoCorrect engine | Have | P0 | Batch 1 |
| AutoFormat... | Have | P2 | Fix round: Tools > AutoFormat... runs the AutoCorrect replacements as a batch pass over the whole document through the command layer, one undo step (unit + E2E) |
| Thesaurus | Partial | P2 | Batch 7: Tools > Thesaurus... (Shift+F7). resources/thesaurus.txt holds 118 entries (word: syn1, syn2, ...). Win95 dialog, Spelling pattern: current word at caret/selection, synonyms sunken list, Look Up field, Replace (one undo step, re-seeds on the replacement), Close. Missing word shows "(no thesaurus entry)". One static entry list (no parts of speech, no Meanings grouping) |
| Mail Merge... | Partial | P2 | Fix round 2: DISABLED (grayed, enabled=false) — no merge engine / data source, so Word-style unavailable, not a stub promising behavior. Dispatch removed; the old Helper dialog code stays unreachable while disabled |
| Options/Preferences | Have | P2 | Fix round: Tools > Options... is a real Win95 dialog with live checkboxes: "Show text boundaries" (docSettings.showTextBoundaries), "Spell check as you type" (new toggle gating squiggle render/cache; Spelling dialog unaffected), "AutoCorrect as you type" (existing AutoCorrect flag). Options flags persist with the document |

## Table
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Insert/Delete rows & columns | Have | P0 | |
| Merge / Split cells | Have | P1 | |
| Table borders rendering | Have | P1 | test_border_rendering |
| Sort / Formula / AutoFormat | Partial | P2 | Batch 7: Table > Sort Ascending/Descending sorts ALL rows by the caret's column (no header concept in the model, so no row pinned) — numeric-aware, numbers before text, case-insensitive text, stable, merged tables refuse. Table > Formula... opens a dialog prefilled =SUM(ABOVE), supports SUM/AVERAGE/COUNT over ABOVE/LEFT, skipping gaps and non-numeric cells, writing the result into the caret cell. Batch 8: the table undo is a stack now (one entry per op), so Sort/Formula/cell runs undo in order instead of only the latest op. Not covered: MIN/MAX, cell references, multi-column sort, header pinning; no table redo |
| Cell editing | Have | P0 | Batch 8: click in a cell places a cell caret (body caret hides); typing / Backspace / Delete / Enter (newline) edit the cell through the command layer, one undo step per typing run (coalesced, stacked with Tab-append/Sort/Formula entries). Tab / Shift+Tab move cells, Tab in the last cell appends a row; Left/Right cross cells at edges, Up/Down cross rows on the first/last visual line (single-line cells always cross). Cell text wraps inside the cell and the row grows from the wrapped lines; the cell caret is a blinking bar, shift+arrows / drag select within the cell (navy, typing replaces). Save/load round-trips tables (saveDocumentEx used to drop them entirely — fixed by routing save/load/autosave/startup through the WithTables variants). Word count excludes cell text (buffer stats only). E2E `pass_table_cell_editing.e2e`, `pass_table_cell_wrap.e2e`; unit tests in `test_table.cpp` `[cell-edit]`. Not covered: no AutoCorrect / smart quotes / @ chips in cells; formatting commands (shortcuts, toolbar, Format menu, Repeat) are no-ops while a cell is active — cells have no formatting model yet; selections cannot span cells; merged masters spanning rows count toward their first row's height only |

## Window
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Window menu (multi-document) | Have | P1 | Window menu between Table and Help, Word 95 order: New Window (launches a second instance of our executable onto the same file via std::system; no file = plain new instance), Arrange All disabled (single-window app), Split, "1 <current file>" ENABLED and CHECKED; clicking the current-doc item is a no-op |
| Split pane | Have | P1 | View > Split View + Window > Split, both toggle splitViewEnabled |

## Help
| Feature | Status | Priority | Notes |
|---|---|---|---|
| About / Keyboard Shortcuts (F1) | Have | P2 | |
| Help Topics / Answer Wizard / Office Assistant | Missing | P2 | never (Clippy is post-95) |

## Chrome (toolbars / ruler / status bar)
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Standard toolbar (icons) | Partial | P0 | Batch 3 + fix round: Word 95 Standard order — New Open Save \| Print Print Preview Spelling (ABC + green check icon) \| Cut Copy Paste FormatPainter \| Undo Redo \| ¶ \| Zoom (dropdown 50/75/100/125/150/200%, wired to zoomLevel) \| Insert Hyperlink / Insert Image / Insert Table. Format Painter: click copies the current style and arms; the next click in text applies it and disarms (command layer; unit + E2E). Subset: the buffer carries one document-global TextStyle, not per-range styles, so apply sets the global style like Bold/Italic do. Show/Hide ¶ (pressed state) renders a ¶ glyph at each line end in the text pass, hidden in Print Preview. Fix round 2: Zoom sits immediately after the ¶ group (was pinned to the right edge); Insert Hyperlink runs the Insert > Hyperlink... command, Insert Image runs Insert > Image..., Insert Table opens the Table menu dropdown (no grid popup — the menu is the Word 95 entry) |
| Formatting toolbar (style/font/size/BIU/align) | Partial | P0 | present; font list = 2 families |
| Title bar / menu bar / bevels | Have | P0 | Win95 theme, drop shadows, focus ring |
| Scrollbars (Win95 style) | Partial | P1 | verify |

## Docs-era (Win95-styled)
Batch 5 (owner direction): after Word 95 parity, a few Google Docs
features people like, dressed as Windows 95. These are NOT Word 95
features; they live in this section so the parity counts above stay
honest about Word 95 itself.

| Feature | Status | Priority | Notes |
|---|---|---|---|
| Margin comments | Have | P1 | Batch 5 item 1: Insert > Comment... (author defaults to "Gabe", createdAt set) anchors a range highlighted pale yellow in the text; in Paged mode a card (white/yellow, 1px border, author + relative date + text) sits in the right margin connected to its anchor row, in Pageless it docks at the right edge of the text area; same-row cards stack; clicking a card moves the caret to the anchor start; Resolve removes the comment. Cards are UI (click_ui `comment_card_<i>` / `comment_resolve_<i>`), hidden in Print Preview and Focus mode; split view shows the top pane's cards. Honest partial: the author line requests Bold weight but the UI font has no bold variant registered, so it renders regular (same as other in-app Bold uses). Unit: relative-date labels; E2E `pass_margin_comments.e2e` (dialog add, card click -> caret 0,0, Resolve -> count 0) |
| Suggesting mode + Revisions dialog | Have | P1 | Batch 5 item 2: Edit menu keeps the Word 95 term "Track Changes". When ON, typed insertions record Insert revisions (green-underlined in text while matching) and Backspace/Delete record Delete revisions. Tools > Revisions... (Win95 shell, Word 95 had one) lists type, author "Gabe", excerpt; Accept / Reject per selected row, Accept All / Reject All wired to the Edit-menu commands; each resolve is one undo step. Honest partial: deletions are NOT kept visible in the text — deleted text vanishes from the buffer (as before) and the Delete revision is a record in the dialog only. Rendering deleted text red-strikethrough in place would need the gap buffer to keep deleted text (per-character deleted flags / overlay shifting caret/selection math), which conflicts fundamentally with the gap buffer, so it is dialog-only by design, not faked. Unit: accept/reject insert/delete, re-anchor rules, reject-all replay, undo; E2E `pass_revisions_dialog.e2e` |
| @ smart chips | Have | P1 | Batch 5 item 3: typing "@" (previous char is start/whitespace) opens a small Win95 listbox popup at the caret: "Today — <date>", "Tomorrow — <date>", "Gabe Ochoa", "Front Door (AI)". Typing filters (case-insensitive substring, e.g. `@ga` -> people only); Arrow/Enter/click selects, Esc or an outside click dismisses. Selection replaces "@" + filter with a chip (label text in the buffer + record in `DocumentComponent.chips`); chips render as text on a raised BUTTON_FACE box in the render pass (offset-aware, same column mapping as squiggles; pageless first-page correct). Backspace at a chip's end deletes the whole chip through the command layer (one undo step). Limits: chips are offset-anchored — an edit inside a chip orphans it (render skips mismatched chips); caret movement is per-character, not chip-unit. Unit: popup filtering, chip insert/delete/re-anchor, orphaning; E2E `pass_smart_chips.e2e` |
| Version History | Have | P1 | Batch 5 item 4: every successful Save / Save As also writes a snapshot `<settings dir>/wordproc_versions/<sanitized doc name>/<YYYYmmdd-HHMMSS>.txt` (same-second saves get a `-1`/`-2` suffix, never overwrite) plus an index line (time, words, chars) in the doc dir's `index.txt`. File > Version History... (after Save As) lists snapshots newest-first (time + word count); Restore replaces the buffer through the command layer (one undo), marks dirty; Close. Footer documents the flow: viewing old text = Restore + Undo. In test mode snapshots go to `output/wordproc_versions` (never into the checkout). Unit: naming/sanitize/index/same-second/list; E2E `pass_version_history.e2e` (save, type more, restore, original text back) |
| Live word count | Have | P1 | Batch 5 item 5: status bar left cluster gains a "Words: N" segment after the Ln/Col group, updating as you type, from buffer stats. The O(n) `TextBuffer::stats()` is cached on the status-bar system and recomputed only when the buffer version changes, so the per-frame status render stays O(1). Bench guard: `make bench-unit` class unchanged (no benchmark touched). E2E `pass_live_word_count.e2e` (type 3 words -> "Words: 3", add a word, delete it) |

## Counts (from tables above, 2026-10-19)
**Have 51 · Partial 14 · Missing 2** (after Batch 8: Table cell editing
moves Missing -> Have) (67 grouped rows; several rows bundle e.g.
Cut/Copy/Paste).
Supersedes the front-door first-reply estimate (31/11/13).
Ranking: Batch 1 = Tools spell trio (squiggles + F7 dialog + AutoCorrect) —
highest daily value, engine already exists. Batch 2 = app chrome (Word 95
ruler + drag, paged framing verify, Window menu). Batch 3 = Print Preview +
Print, Standard toolbar (Print Preview + Spelling), period Tools/View menus,
Grammar dialog, menu typing gate. Then: Normal/Outline views, Columns,
Borders/Shading dialog, full Font dialog + more families, Date/Time insert,
toolbar icon fidelity (¶ button, Insert Table grid).
Also Batch 3: vendored fmt (11.2.0) verified compiling on system clang 17
and zig-bundled clang 21.1.8 (header + format-inl.h) — already newer than
the kart-afterhours sibling (8.0.1), no bump needed; the mailboy
consteval breakage does not affect it.
Batch 7 re-verified the same: vendored fmt 11.2.0 (same as the mailboy
sibling; kart uses an older fmt with no base.h) compiles clean as
header-only (FMT_HEADER_ONLY, as the app consumes it) with system
clang++ and with `zig c++` (bundled clang 21.1.8) — no consteval
errors, no bump. One commit per item, no push.
