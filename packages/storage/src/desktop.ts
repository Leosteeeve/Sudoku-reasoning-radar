import type { MigrationReport, PuzzleRecord } from "./index.ts";

const DESKTOP_MIGRATION_MARKER = "desktop-windows-pipe-v1";

export type DesktopLegacyImportResult =
  | { version: 1; status: "not-found" }
  | { version: 1; status: "ok"; records: PuzzleRecord[]; errors: Array<{ line: number; message: string }> };

export interface DesktopLegacySource {
  import(): Promise<DesktopLegacyImportResult>;
}

export interface DesktopMigrationStore {
  hasMigration(marker: string): Promise<boolean>;
  importPuzzles(records: PuzzleRecord[]): Promise<void>;
  markMigration(marker: string): Promise<void>;
}

function errorMessage(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export async function migrateLegacyDesktopPuzzles(
  source: DesktopLegacySource,
  store: DesktopMigrationStore,
): Promise<MigrationReport> {
  if (await store.hasMigration(DESKTOP_MIGRATION_MARKER)) {
    return { imported: 0, skipped: true, errors: [] };
  }

  try {
    const result = await source.import();
    if (result.status === "not-found") {
      await store.markMigration(DESKTOP_MIGRATION_MARKER);
      return { imported: 0, skipped: false, errors: [] };
    }

    await store.importPuzzles(result.records);
    await store.markMigration(DESKTOP_MIGRATION_MARKER);
    return {
      imported: result.records.length,
      skipped: false,
      errors: result.errors.map((error) => `line ${error.line}: ${error.message}`),
    };
  } catch (error) {
    return { imported: 0, skipped: false, errors: [errorMessage(error)] };
  }
}
