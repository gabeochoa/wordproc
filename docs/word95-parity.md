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
| New from Template... | Partial | P1 | template dialog exists; no shipped .dot-style templates |
| Export RTF / HTML / PDF | Have | P1 | File menu exports; native .doc binary I/O Missing below |
| Save/open .doc (Word binary) | Missing | P2 | RTF is the interchange path |
| Save/open .rtf / .txt | Have | P0 | document_io + tests |
| Page Setup (margins/paper) | Partial | P0 | dialog + margins state; verify paged render honors all fields |
| Print | Have | P1 | File > Print... / Ctrl+P / toolbar Print: PDF via export_pdf to a temp file, then `lp`; in test mode the PDF is written but `lp` never runs. Honest partial: export_pdf is single-page with overflow past the bottom margin, so Print prints what's supported, not a true multi-page job |
| Print Preview | Have | P1 | Batch 3 + fix round: read-only mode (File > Print Preview, toolbar button); forces Paged, one page on gray workspace. Printable content ONLY: no spell squiggles, caret, selection, page-break indicator, or line numbers in preview (gated in the render path; paged editing view unchanged). Preview toolbar is ONE compact ~28px row of beveled Win95 buttons: [Print icon] [Prev Page] [Next Page] [Page X of Y] [Zoom dropdown 50/75/100/150/200%] [Close Print Preview] — the old bar was double height (both toolbars). Ruler hidden in preview (Word default; showRuler flag untouched, restored on exit) and Formatting bar gone, so chrome above the text shrinks (text_area_y 153 -> 96 at ui_scale 1.2). Status bar shows "Page X of Y" + zoom % instead of Ln/Col/At and the REC/MRK/EXT/OVR block. Esc/Close restores the previous mode; typing/scroll gated; zoom recomputes the pagination estimate. Page count is an ESTIMATE from geometry (page text height / scaled line height) — no real pagination exists, so headings/spacing shift true breaks |
| Exit / MRU list | Have | P2 | — |

## Edit
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Undo / Redo | Have | P0 | CommandHistory, multi-level |
| Cut / Copy / Paste | Have | P0 | incl. toolbar + context menu |
| Select All | Have | P0 | |
| Find / Replace / Find Next-Prev | Have | P0 | dialog + F3/Shift+F3 |
| Go To / Bookmarks | Partial | P1 | bookmark dialog + Go To Bookmark |
| Track Changes (revise mode) | Partial | P1 | toggle + accept/reject all; no per-revision UI |
| AutoCorrect | Have | P0 | replace-as-type + TWo-caps + sentence cap; Tools dialog (Batch 1) |
| Repeat (F4) | Missing | P2 | |

## View
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Normal view | Missing | P1 | only pageless/paged |
| Page Layout view | Have | P0 | Paged Mode; framing verified headless screenshot 2026-10-18 (white Letter page, gray workspace, text inside 1in margins). ONE scale (Batch 2 fix round): page width = 8.5 × ruler ppi, ppi = 72 × (fit × zoom), margins/page/ruler/indents all derive from it; mode switch flashes transient status-bar text, not a toast |
| Outline view | Partial | P1 | Show Outline pane exists; not a Word outline view |
| Zoom (in/out/reset, %) | Have | P0 | Ctrl+Alt +/-/0 |
| Ruler (indents/tabs draggable) | Have | P0 | Word 95 ruler: gray outside margins, white between (pageless too — 1in margins inset the effective area), inch ticks at the ONE display ppi, indent markers beveled (pale face, dark edges, hourglass pair + rectangle base + right triangle, apexes toward centre) at the same scale as text indents; left/first-line/right markers drag to set indents (one undo step per drag); View > Ruler toggles; tab-stop marks at default stops (not draggable) |
| Toolbars show/hide | Missing | P2 | Standard+Formatting always on |
| Status bar | Partial | P0 | Word 6-style; page/sec/Ln/Col present, verify accuracy. Transient StatusComponent messages (e.g. "Paged mode" / "Pageless mode" after a View mode switch) replace the stats line for 3s — no floating toast for mode switches |
| Header/Footer view | Partial | P1 | Insert Header/Footer entries exist |

## Insert
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Page/Section Break | Have | P0 | |
| Table... | Have | P0 | grid insert + Table menu |
| Picture... | Have | P1 | image insert + layout/wrap |
| Symbol / Special Character... | Partial | P1 | dialog exists; date/time insert Missing |
| Date & Time... | Missing | P1 | |
| Header / Footer / Page Number | Have | P0 | |
| Footnote | Have | P2 | |
| Hyperlink / Bookmark / Comment | Have | P1 | comment is also a Docs-era feature, kept |
| Table of Contents | Partial | P2 | entry exists; field update semantics TBD |
| Equation / Shapes | Have | P2 | beyond Word 95 basic drawing |

## Format
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Font... dialog (family/size/effects) | Partial | P0 | Formatting toolbar dropdowns; only 2 families wired |
| Bold/Italic/Underline/Strike/Super/Sub | Have | P0 | |
| Alignment L/C/R/Justify | Have | P0 | |
| Indents & Tabs (ruler + dialog) | Partial | P0 | ruler drag sets indents (one undo step) + indent keys + Tab Width dialog; no full Paragraph dialog |
| Bullets & Numbering | Have | P0 | multi-level list indent |
| Paragraph... (spacing/line spacing) | Partial | P1 | menu increments, no full dialog |
| Borders and Shading... | Missing | P1 | table borders render; no dialog |
| Columns... | Missing | P1 | |
| Styles (Normal/Title/Headings 1-6) | Have | P0 | style dropdown + Ctrl+Alt+n |
| Drop Cap | Partial | P2 | menu entry exists |

## Tools
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Word Count... | Have | P0 | dialog: words/chars/lines/paragraphs/sentences (verified in menu_ui_system.h) |
| Spelling (F7 dialog) | Have | P0 | Batch 1: dialog + suggestions + Change All |
| Red squiggle spell check as you type | Have | P0 | Batch 1: visible-lines cache, reviewer 8/10 |
| Grammar check | Have | P2 | Batch 3: Tools > Grammar... dialog (read-only list: rule + excerpt + message + suggestion; Close only, no auto-fix). Engine covers repeated word, capitalization, double space, common errors. Tools menu is Word 95 order (Spelling, Word Count, AutoCorrect, sep, Grammar, sep); UI Scale... moved to View bottom; disabled Preferences... stub removed |
| AutoCorrect engine | Have | P0 | Batch 1 |
| Thesaurus | Missing | P2 | |
| Mail Merge... | Missing | P2 | stub OK per plan |
| Options/Preferences | Partial | P2 | Preferences disabled in menu |

## Table
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Insert/Delete rows & columns | Have | P0 | |
| Merge / Split cells | Have | P1 | |
| Table borders rendering | Have | P1 | test_border_rendering |
| Sort / Formula / AutoFormat | Missing | P2 | |

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
| Standard toolbar (icons) | Partial | P0 | Batch 3: Print Preview + Spelling buttons added in Word 95 Standard order (Print, Print Preview, Spelling, Cut/Copy/Paste, Undo/Redo), drawn icons. Show/Hide ¶ NOT added: pilcrow rendering in the text pass is not trivial. Insert Table grid button NOT added: needs a grid popup that does not exist |
| Formatting toolbar (style/font/size/BIU/align) | Partial | P0 | present; font list = 2 families |
| Title bar / menu bar / bevels | Have | P0 | Win95 theme, drop shadows, focus ring |
| Scrollbars (Win95 style) | Partial | P1 | verify |

## Counts (from tables above, 2026-10-18)
**Have 37 · Partial 17 · Missing 11** (after Batch 3) (65 grouped rows; several rows bundle
e.g. Cut/Copy/Paste). Supersedes the front-door first-reply estimate (31/11/13).
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
