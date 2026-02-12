# Data Loss Prevention

Guards against document corruption and data loss: atomic file writes (write to temp then rename), backup-before-save, file integrity checksums, and graceful handling of disk-full and permission errors.

## Status

Autosave with crash recovery exists. Atomic writes, backup-before-save, and integrity verification are not yet implemented.
