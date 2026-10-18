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
