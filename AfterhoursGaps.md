# Afterhours Gaps

Tracking any Afterhours library changes we might need, plus app-side workarounds.

## Current gaps

- **Test input hooks**: `src/external.h` expects test input helpers for automated input.
  - **Workaround**: only the forward declarations are copied for now; no test
    input injection is used yet.

## Notes

- No vendor/afterhours changes are required at this stage.
