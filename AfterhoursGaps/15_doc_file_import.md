# .doc File Import

## Problem
Microsoft Word `.doc` files use a proprietary binary format (OLE Compound
Document / CFBF). Importing these files requires parsing both the container and
Word-specific binary streams.

## Complexity
- Parse OLE2/CFBF container and locate streams
- Decode Word binary record structures
- Handle legacy encodings and embedded objects

## Candidate Libraries
- `antiword` (GPL, C): extracts text
- `wv` / `wvWare` (GPL): more complete Word parsing
- `libole2` / `pole`: OLE container parsing
- Python `python-docx` (MIT) is `.docx` only

## Current Workaround
For v0.1, recommend users convert `.doc` to `.txt` or `.docx` before importing.

## Future Option
Add `wvWare` or `antiword` as an optional dependency for v0.2+ `.doc` import.

