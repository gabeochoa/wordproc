# Code Folding

Collapse and expand regions of text/code. Fold detection is external (LSP, parser, or app logic provides fold ranges) -- the UI widget just renders the fold state.

## Status

Not yet implemented.

## Decisions

- **Fold detection**: External to the UI widget. The app/LSP/parser provides fold ranges; AfterHours just renders them
- **Gutter indicator**: Fold arrow appears on hover only (down = expanded, right = folded)
- **Folded display**: Inline ellipsis marker (...) on the fold line with a count of hidden lines
- **Keyboard**: Yes, shortcuts for fold/unfold current region, fold all, unfold all. Exact keys TBD
