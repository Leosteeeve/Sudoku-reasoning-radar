# Privacy and local data

Sudoku Reasoning Radar has no accounts, telemetry, cloud sync, leaderboard, or daily challenge service. Puzzles, sessions, preferences, migration markers, and backups remain on the device in IndexedDB or files explicitly selected by the user.

OCR runs only after the user invokes the image-import command. Before that command, the helper process is not spawned and OCR resources are not accessed. Selected images and recognition results are not uploaded by the application.

The desktop beta may perform a once-daily GitHub Releases update check. It sends the ordinary HTTPS request metadata needed to reach GitHub but no puzzle, session, image, filename, or stable user identifier. Users can block that request without losing local solving features.

Backups are plain local JSON and may contain puzzle history. Store and share them with the same care as any personal local file.
