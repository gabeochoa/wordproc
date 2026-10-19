# Visual reviews (other-model)

Every milestone: headless screenshots (`--headless --test-mode
--test-script=...`, see `src/testing/deferred_screenshot.h`) are uploaded to
pixelcloud and graded by a reviewer subagent on a DIFFERENT model than the
implementer, against real Word 95 reference screenshots it fetches itself.

## 2026-10-18 — Batch 1 (spell squiggles, Spelling dialog, AutoCorrect, Word Count)

- Implementer: Muse (tbh-1015). Reviewer: gpt-6-sol (session 7fe56e74).
- Screenshots: b1_01_squiggles https://pxl.cl/gBGCq · b1_02_tools_menu
  https://pxl.cl/gBGCw · b1_03_spelling_dialog https://pxl.cl/gBGCF ·
  b1_04_autocorrect_dialog https://pxl.cl/gBGCQ · b1_05_word_count
  https://pxl.cl/gBGDf
- References used by reviewer: real Word 95 captures from
  https://winworldpc.org/product/microsoft-word/95

**Verdict: does not yet read as Word 95.** Scores: squiggles 3/10, Tools menu
2/10, Spelling 1/10, AutoCorrect 0.5/10, Word Count 1/10.

Top findings (full list in reviewer session 7fe56e74):
1. All three dialogs use a modern modal shell (dark scrim, rounded card,
   navy pill buttons) — replace with square #c0c0c0 Win95 dialogs, navy
   caption bar, beveled gray buttons, sunken fields/lists.
2. Spelling geometry wrong: Word 95 has sunken "Not in dictionary" field +
   sunken Suggestions list on the left, action buttons stacked on the right.
3. Document is an edge-to-edge white canvas: no visible page/margins on gray,
   no ruler visible in pageless view. (Queued: chrome batch.)
4. AutoCorrect looks like a modern settings modal; needs native checkboxes
   plus Replace/With fields and the replacement list.
5. Toolbar density/iconography, missing Window menu, title pattern — chrome
   batch. Tools menu contains non-period UI Scale/Preferences — chrome batch.
6. "test" squiggled (dictionary has only ~742 words) and "world" missing
   from suggestions for "wrold" (plain Levenshtein ranks transposition = 2).

Fix round applied: dialog shell + Spelling geometry + Word Count layout +
AutoCorrect list (Task A) and dictionary expansion + Damerau suggestion
ranking (Task B). Re-shoot + re-review follow below.

### Round 2 (after dialog-shell fix, commits 4c3f05b + 83db729)

Screenshots: https://pxl.cl/gBLSx https://pxl.cl/gBLSH https://pxl.cl/gBLSS
https://pxl.cl/gBLT1 — Scores: squiggles 8/10, Spelling 6/10, AutoCorrect
3/10, Word Count 6.5/10. Verdict: not yet — AutoCorrect still a mockup
(right-aligned ticks, four-column text wall), field/list sunken edges and
oversized buttons remain. Fix round 3 dispatched (native checkboxes,
sunken Replace|With list + editable entries, narrower buttons).

### Round 3 (commit 4a9d895)

AutoCorrect rebuilt as a genuine dialog (native checkboxes, sunken
Replace|With list, editable entries with Add/Delete, OK/Cancel); 2px sunken
edges; 90px buttons (root cause: `h720()` widths resolve against screen
width — dialogs now use `w1280()` for widths).

### Round 3b — FINAL, Batch 1 surfaces

Reviewer (gpt-6-sol): squiggles 8.5/10, Spelling 8/10, AutoCorrect 7.5/10,
Word Count 8/10 — **"READS AS WORD 95 for these surfaces."** The stacked
Word Count capture in round 3 was a capture-script bug (stale
`autocorrect_close` debug name), fixed and re-captured clean:
https://pxl.cl/gBQ1j (round-3 set: gBPR6/gBPRv/gBPRF/gBPRX).
Minor non-blockers noted: tiny notch in Word Count caption border,
AutoCorrect list header separation. App chrome (ruler/page/toolbars/Window
menu) deliberately unscored — that's Batch 2.

## 2026-10-18 — P0 UX fixes (owner-reported, items 2/5/6/7/8/10)

Reviewer: gpt-6-sol vs WinWorld Word 95 references. Before/after screenshots
in /tmp/p0_before, /tmp/p0_after (after uploads: pxl.cl/gC9p9 idle,
gC9pn Format menu, gC9pT selection, gC9q0 dialog buttons, gC9qr tooltip,
gC9qD File menu long recent).

Scores: menu labels 7/10, selection 9.5/10, beveled-button label 9/10,
tooltip 9/10, shortcut inset 8.5/10, File-menu width/ellipsis 8/10.
**Verdict: pass for all six scoped UX fixes.** Polish noted (not blockers):
menu-bar File lacks the Word app icon/padding and labels sit in boxed
separators; MRU entries read "Recent: <unix path>" instead of Word 95's
numbered filenames — queued with the chrome batch.

## 2026-10-18 — Batch 2 (ruler, paged page, Window menu) + menu-bar inset

Round 1 scores: menubar inset 6/10 (symmetric but overspaced, open state
wrong), ruler 5/10 (scale mismatch, black markers), paged page 5/10 (scale,
toast), Window menu 4/10 (disabled doc entry).
Fix round (87094e3 one scale source, 547f056 beveled markers, 2f78765
status-bar mode flash, 168ab11 Window menu, 570b714 visible √ checkmark —
Roboto has no U+2713 glyph): menubar 8.5 READS, ruler 8 READS (page
358px = 8.5in × 42ppi, counted), Window 8.5 READS; paged page 6/10 —
text not scaled with page zoom + margin boundary guides shown by
default. Round 2 dispatched for those two.

Round 2 (d953a4c text scales with page zoom, b522188 text boundaries off
by default): paged page 8.5/10 — **READS AS WORD 95.** Glyph ink 6px at
10pt/42ppi (expected 5.8). All Batch 2 surfaces pass: menubar 8.5,
ruler 8, paged 8.5, Window 8.5.

## 2026-10-18 — Batch 3 (Print Preview, toolbar, Tools, Grammar)

Rounds: initial 3.5/4/2/5 -> fix round (printable-only preview, real
Grammar checker, dense Tools, ABC✓/painter/¶/zoom toolbar) -> round 2
(effective zoom 66%, Mail Merge grayed, Zoom placement, +459 dictionary
words; File's framed look traced to afterhours focus ring on recycled
entity ids) -> round 3 (Grammar re-scans after Change; focus parked on
idle frames, File flat). FINAL: Print Preview 8, Tools 7.5, Grammar 8,
Standard toolbar 7.5 — **Batch 3 PASS, all read as Word 95.**

## 2026-10-18 — Batch 4 (Date/Time, Templates, Font, Outline, Columns)

Rounds: Date 7, Template 3->7, Font 4->6->8, Outline 2->6->7.5,
Columns 5.5->6.5->8.5. **Batch 4 PASS — all read as Word 95.**
Notable roots found en route: squiggle cache covered only the
single-column window (columns), Font pending-state desync, outline
level indent unused, "column" simply missing from the dictionary.
Process note: one review round was graded on STALE /tmp captures —
always upload from the round's own screenshot dir.

## 2026-10-18 — Batch 5 (Docs-era features, Win95-styled)

Rounds: comments 4->7.5, inline revisions 1->8 (glyphs were black;
green underline existed but squiggle masked it), Revisions dialog
4->4.5->7.5 (list ran under buttons; rows now grouped), chips 5.5->7
(adjacent chips merged into one box; popup click never fired),
Version History 4->7.5, @ popup 7.5. **Batch 5 PASS.**
Timezone note: chips/history use OS localtime; review host is ET,
owner PT — format judged, zone not penalized.

## 2026-10-19 — Batch 6 (Page Setup, Paragraph, Borders, Normal, Status)

Page Setup 8, Paragraph 4.5->7.5, Borders 7->7.5, Normal 4.5->8,
Status 6->8.5. **Batch 6 PASS.** Dictionary grew +403 words mined by
frequency from the public-domain test books.

## 2026-10-19 — Batch 7 (Thesaurus, Repeat, toolbar toggles, tables)

Thesaurus 6->7.5, Formatting-toggle 8.5, table render/sort 6->7.5.
**Batch 7 PASS.** Big find: table RENDERING was dead code (tables were
invisible) — now wired. All 98 info toasts swept to status-bar flashes;
only error toasts remain. Typing into table cells is still
unimplemented (cells seeded via test hooks only) — Batch 8.

## 2026-10-19 — Batch 8 (table cell editing)

cell_editing 8, cell_wrap 8 — **PASS.** Tables also persisted on
save for the first time (serializers had no callers). Polish round:
active-cell outline navy on all edges (neighbors' gridlines were
overpainting it), status flashes expire on next keystroke.
