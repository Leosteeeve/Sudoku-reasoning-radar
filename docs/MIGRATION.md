# Migration guide

## v0.3.0 to v0.4.0-beta.1

The beta copies recognized legacy puzzles into the versioned local store. Web migration reads `sudoku_reasoning_radar_last`; desktop migration asks the narrow Electron bridge to discover legacy puzzle files. Both migrations are idempotent and deduplicate normalized 81-digit puzzles.

Migration never deletes or modifies legacy data. A migration marker is written only after a successful copy or a confirmed not-found result. If a read, validation, or write fails, the marker is not written and the next launch can retry safely.

Before migration, export a `.srr.json` backup when the previous version supports it. After launch, confirm the puzzle library and current session, then export a new backup. Keep the old application directory and legacy files until at least one stable release has been validated.

v0.3.0 downloads and old data are retained for at least one stable release cycle after this beta. The project will not remove the older downloads as part of the beta rollout.
