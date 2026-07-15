export interface PuzzleRecord {
  puzzle: string;
  name?: string;
  difficulty?: string;
  solution?: string;
  elapsedMs?: number;
  seed?: number;
  source?: string;
  createdAt?: string;
  updatedAt?: string;
}

export interface SessionRecord {
  id: string;
  puzzle: string;
  values: number[];
  noteMasks: number[];
  mode: string;
  trace: unknown[];
  currentStep: number;
  elapsedMs: number;
  savedAt: string;
}

export interface SettingsRecord {
  id: string;
  language?: "zh" | "en";
  theme?: "light" | "dark";
  highContrast?: boolean;
  reducedMotion?: boolean;
  migrationSource?: string;
  migrationVersion?: number;
  migratedAt?: string;
}

export interface StorageService {
  listPuzzles(): Promise<PuzzleRecord[]>;
  getPuzzle(puzzle: string): Promise<PuzzleRecord | undefined>;
  upsertPuzzle(record: PuzzleRecord): Promise<void>;
  importPuzzles(records: PuzzleRecord[]): Promise<void>;
  saveCurrentSession(session: SessionRecord): Promise<void>;
  loadCurrentSession(): Promise<SessionRecord | undefined>;
  setSettings(settings: SettingsRecord): Promise<void>;
  getSettings(id: string): Promise<SettingsRecord | undefined>;
  listSessions(): Promise<SessionRecord[]>;
  listSettings(): Promise<SettingsRecord[]>;
  importBackup(value: unknown): Promise<void>;
  exportBackup(appVersion: string, exportedAt?: string): Promise<string>;
  close(): void;
}

export interface MigrationStore {
  hasMigration(marker: string): Promise<boolean>;
  upsertPuzzle(puzzle: PuzzleRecord): Promise<void>;
  markMigration(marker: string): Promise<void>;
}

export interface LegacyValueSource {
  get(key: string): string | null | undefined;
}

export interface MigrationReport {
  imported: number;
  skipped: boolean;
  errors: string[];
}

const WEB_LEGACY_KEY = "sudoku_reasoning_radar_last";
const WEB_MIGRATION_MARKER = "web-local-storage-v1";

export function normalizePuzzle(value: string): string {
  const normalized = value.replace(/\s/g, "").replaceAll(".", "0");
  if (!/^[0-9]{81}$/.test(normalized)) {
    throw new Error("Puzzle must contain exactly 81 digits, dots, or whitespace");
  }
  return normalized;
}

function errorMessage(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export async function migrateLegacyWebPuzzle(
  source: LegacyValueSource,
  store: MigrationStore,
): Promise<MigrationReport> {
  if (await store.hasMigration(WEB_MIGRATION_MARKER)) {
    return { imported: 0, skipped: true, errors: [] };
  }
  const raw = source.get(WEB_LEGACY_KEY);
  if (raw == null) return { imported: 0, skipped: true, errors: [] };
  try {
    const puzzle = normalizePuzzle(raw);
    await store.upsertPuzzle({ puzzle, source: "web-local-storage" });
    await store.markMigration(WEB_MIGRATION_MARKER);
    return { imported: 1, skipped: false, errors: [] };
  } catch (error) {
    return { imported: 0, skipped: false, errors: [errorMessage(error)] };
  }
}

export {
  IndexedDbStorage,
  openStorage,
  openVersionedDatabase,
  type OpenStorageOptions,
} from "./indexeddb.ts";
export {
  mergePuzzleRecords,
  parseLegacyPipeText,
  type LegacyParseError,
  type LegacyParseResult,
} from "./legacy.ts";
export {
  createBackup,
  parseBackup,
  serializeBackup,
  type BackupData,
  type BackupV1,
} from "./backup.ts";
export {
  migrateLegacyDesktopPuzzles,
  type DesktopLegacyImportResult,
  type DesktopLegacySource,
  type DesktopMigrationStore,
} from "./desktop.ts";
