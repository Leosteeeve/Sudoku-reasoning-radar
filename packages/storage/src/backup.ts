import {
  normalizePuzzle,
  type PuzzleRecord,
  type SessionRecord,
  type SettingsRecord,
} from "./index.ts";
import { mergePuzzleRecords } from "./legacy.ts";

export interface BackupV1 {
  schemaVersion: 1;
  puzzles: PuzzleRecord[];
  sessions: SessionRecord[];
  settings: SettingsRecord[];
  exportedAt: string;
  appVersion: string;
}

export interface BackupData {
  puzzles: PuzzleRecord[];
  sessions: SessionRecord[];
  settings: SettingsRecord[];
}

function record(value: unknown, path: string): Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) throw new Error(`${path} must be an object`);
  return value as Record<string, unknown>;
}

function onlyKeys(value: Record<string, unknown>, allowed: readonly string[], path: string): void {
  const unknown = Object.keys(value).find((key) => !allowed.includes(key));
  if (unknown) throw new Error(`${path} contains unknown field ${unknown}`);
}

function text(value: unknown, path: string, allowEmpty = false): string {
  if (typeof value !== "string" || (!allowEmpty && value.trim() === "")) throw new Error(`${path} must be a non-empty string`);
  return value;
}

function optionalText(value: unknown, path: string): string | undefined {
  return value === undefined ? undefined : text(value, path, true);
}

function date(value: unknown, path: string): string {
  const result = text(value, path);
  if (!Number.isFinite(Date.parse(result))) throw new Error(`${path} must be a valid date`);
  return result;
}

function optionalDate(value: unknown, path: string): string | undefined {
  return value === undefined ? undefined : date(value, path);
}

function integer(value: unknown, min: number, max: number, path: string): number {
  if (!Number.isInteger(value) || (value as number) < min || (value as number) > max) throw new Error(`${path} must be an integer from ${min} to ${max}`);
  return value as number;
}

function jsonValue(value: unknown, path: string): void {
  if (value === null || typeof value === "string" || typeof value === "boolean") return;
  if (typeof value === "number" && Number.isFinite(value)) return;
  if (Array.isArray(value)) {
    value.forEach((entry, index) => jsonValue(entry, `${path}[${index}]`));
    return;
  }
  if (typeof value === "object") {
    for (const [key, entry] of Object.entries(value as Record<string, unknown>)) jsonValue(entry, `${path}.${key}`);
    return;
  }
  throw new Error(`${path} must contain only JSON values`);
}

function validatePuzzle(value: unknown, path: string): PuzzleRecord {
  const input = record(value, path);
  onlyKeys(input, ["puzzle", "name", "difficulty", "solution", "elapsedMs", "seed", "source", "createdAt", "updatedAt"], path);
  const rawPuzzle = text(input.puzzle, `${path}.puzzle`);
  const puzzle = normalizePuzzle(rawPuzzle);
  if (rawPuzzle !== puzzle) throw new Error(`${path}.puzzle must be a normalized 81-digit puzzle`);
  const result: PuzzleRecord = { puzzle };
  const name = optionalText(input.name, `${path}.name`);
  const difficulty = optionalText(input.difficulty, `${path}.difficulty`);
  const solution = optionalText(input.solution, `${path}.solution`);
  if (solution !== undefined && !/^[1-9]{81}$/.test(solution)) throw new Error(`${path}.solution must contain 81 digits from 1 to 9`);
  if (name !== undefined) result.name = name;
  if (difficulty !== undefined) result.difficulty = difficulty;
  if (solution !== undefined) result.solution = solution;
  if (input.elapsedMs !== undefined) result.elapsedMs = integer(input.elapsedMs, 0, Number.MAX_SAFE_INTEGER, `${path}.elapsedMs`);
  if (input.seed !== undefined) result.seed = integer(input.seed, 0, 0xffff_ffff, `${path}.seed`);
  const source = optionalText(input.source, `${path}.source`);
  if (source !== undefined) result.source = source;
  const createdAt = optionalDate(input.createdAt, `${path}.createdAt`);
  const updatedAt = optionalDate(input.updatedAt, `${path}.updatedAt`);
  if (createdAt !== undefined) result.createdAt = createdAt;
  if (updatedAt !== undefined) result.updatedAt = updatedAt;
  return result;
}

function numberArray(value: unknown, max: number, path: string): number[] {
  if (!Array.isArray(value) || value.length !== 81) throw new Error(`${path} must contain 81 entries`);
  return value.map((entry, index) => integer(entry, 0, max, `${path}[${index}]`));
}

function validateSession(value: unknown, path: string): SessionRecord {
  const input = record(value, path);
  onlyKeys(input, ["id", "puzzle", "values", "noteMasks", "mode", "trace", "currentStep", "elapsedMs", "savedAt"], path);
  const id = text(input.id, `${path}.id`);
  const rawPuzzle = text(input.puzzle, `${path}.puzzle`);
  const puzzle = normalizePuzzle(rawPuzzle);
  if (rawPuzzle !== puzzle) throw new Error(`${path}.puzzle must be normalized`);
  const values = numberArray(input.values, 9, `${path}.values`);
  const noteMasks = numberArray(input.noteMasks, 511, `${path}.noteMasks`);
  const mode = text(input.mode, `${path}.mode`);
  if (!Array.isArray(input.trace)) throw new Error(`${path}.trace must be an array`);
  jsonValue(input.trace, `${path}.trace`);
  const trace = structuredClone(input.trace) as unknown[];
  const currentStep = integer(input.currentStep, 0, Math.max(0, trace.length - 1), `${path}.currentStep`);
  const elapsedMs = integer(input.elapsedMs, 0, Number.MAX_SAFE_INTEGER, `${path}.elapsedMs`);
  const savedAt = date(input.savedAt, `${path}.savedAt`);
  return { id, puzzle, values, noteMasks, mode, trace, currentStep, elapsedMs, savedAt };
}

function validateSettings(value: unknown, path: string): SettingsRecord {
  const input = record(value, path);
  onlyKeys(input, ["id", "language", "theme", "highContrast", "reducedMotion", "migrationSource", "migrationVersion", "migratedAt"], path);
  const result: SettingsRecord = { id: text(input.id, `${path}.id`) };
  if (input.language !== undefined) {
    if (input.language !== "zh" && input.language !== "en") throw new Error(`${path}.language is invalid`);
    result.language = input.language;
  }
  if (input.theme !== undefined) {
    if (input.theme !== "light" && input.theme !== "dark") throw new Error(`${path}.theme is invalid`);
    result.theme = input.theme;
  }
  for (const key of ["highContrast", "reducedMotion"] as const) {
    if (input[key] !== undefined) {
      if (typeof input[key] !== "boolean") throw new Error(`${path}.${key} must be boolean`);
      result[key] = input[key];
    }
  }
  const migrationSource = optionalText(input.migrationSource, `${path}.migrationSource`);
  if (migrationSource !== undefined) result.migrationSource = migrationSource;
  if (input.migrationVersion !== undefined) result.migrationVersion = integer(input.migrationVersion, 1, Number.MAX_SAFE_INTEGER, `${path}.migrationVersion`);
  const migratedAt = optionalDate(input.migratedAt, `${path}.migratedAt`);
  if (migratedAt !== undefined) result.migratedAt = migratedAt;
  return result;
}

export function parseBackup(value: string | unknown): BackupV1 {
  let parsed = value;
  if (typeof value === "string") {
    try { parsed = JSON.parse(value) as unknown; }
    catch { throw new Error("Backup must be valid JSON"); }
  }
  const input = record(parsed, "backup");
  onlyKeys(input, ["schemaVersion", "puzzles", "sessions", "settings", "exportedAt", "appVersion"], "backup");
  if (input.schemaVersion !== 1) throw new Error("Unsupported backup schema version");
  if (!Array.isArray(input.puzzles) || !Array.isArray(input.sessions) || !Array.isArray(input.settings)) throw new Error("Backup puzzles, sessions, and settings must be arrays");
  const puzzleMap = new Map<string, PuzzleRecord>();
  input.puzzles.forEach((entry, index) => {
    const candidate = validatePuzzle(entry, `backup.puzzles[${index}]`);
    const existing = puzzleMap.get(candidate.puzzle);
    puzzleMap.set(candidate.puzzle, existing ? mergePuzzleRecords(existing, candidate) : candidate);
  });
  const sessions = input.sessions.map((entry, index) => validateSession(entry, `backup.sessions[${index}]`));
  const settings = input.settings.map((entry, index) => validateSettings(entry, `backup.settings[${index}]`));
  return {
    schemaVersion: 1,
    puzzles: [...puzzleMap.values()].sort((left, right) => left.puzzle.localeCompare(right.puzzle)),
    sessions: sessions.sort((left, right) => left.id.localeCompare(right.id)),
    settings: settings.sort((left, right) => left.id.localeCompare(right.id)),
    exportedAt: date(input.exportedAt, "backup.exportedAt"),
    appVersion: text(input.appVersion, "backup.appVersion"),
  };
}

export function createBackup(
  data: BackupData,
  metadata: { exportedAt?: string; appVersion: string },
): BackupV1 {
  return parseBackup({
    schemaVersion: 1,
    puzzles: data.puzzles,
    sessions: data.sessions,
    settings: data.settings,
    exportedAt: metadata.exportedAt ?? new Date().toISOString(),
    appVersion: metadata.appVersion,
  });
}

export function serializeBackup(backup: BackupV1): string {
  return `${JSON.stringify(parseBackup(backup), null, 2)}\n`;
}
