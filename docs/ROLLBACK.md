# Rollback guide

1. Export a `.srr.json` backup from v0.4.0-beta.1 and close every Sudoku Reasoning Radar window.
2. Preserve the beta local-data directory; do not delete IndexedDB or migration markers while investigating.
3. Reinstall or unpack the retained v0.3.0 download in a separate directory.
4. Open v0.3.0 against its original data. Migration never deletes or overwrites that legacy source, so rollback does not require reversing a destructive conversion.
5. Keep the beta backup for a later fixed build. Do not import a newer schema into v0.3.0 unless that release explicitly supports it.

v0.3.0 downloads and old data are retained for at least one stable release cycle. If rollback is caused by a migration fault, copy the source and beta data directories before reporting the issue.
