import { normalizePuzzle, type PuzzleRecord } from "./index.ts";

export interface LegacyParseError {
  line: number;
  message: string;
}

export interface LegacyParseResult {
  records: PuzzleRecord[];
  errors: LegacyParseError[];
}

function present(value: unknown): boolean {
  return value !== undefined && value !== null && (typeof value !== "string" || value.trim() !== "");
}

function earlier(left?: string, right?: string): string | undefined {
  if (!left) return right;
  if (!right) return left;
  return Date.parse(left) <= Date.parse(right) ? left : right;
}

function later(left?: string, right?: string): string | undefined {
  if (!left) return right;
  if (!right) return left;
  return Date.parse(left) >= Date.parse(right) ? left : right;
}

export function mergePuzzleRecords(existing: PuzzleRecord, incoming: PuzzleRecord): PuzzleRecord {
  const puzzle = normalizePuzzle(existing.puzzle);
  if (puzzle !== normalizePuzzle(incoming.puzzle)) throw new Error("Cannot merge different puzzles");
  const merged: PuzzleRecord = { puzzle };
  for (const key of ["name", "difficulty", "solution", "elapsedMs", "seed", "source"] as const) {
    const value = present(existing[key]) ? existing[key] : incoming[key];
    if (present(value)) Object.assign(merged, { [key]: value });
  }
  const createdAt = earlier(existing.createdAt, incoming.createdAt);
  const updatedAt = later(existing.updatedAt, incoming.updatedAt);
  if (createdAt) merged.createdAt = createdAt;
  if (updatedAt) merged.updatedAt = updatedAt;
  return merged;
}

export function parseLegacyPipeText(text: string): LegacyParseResult {
  const records = new Map<string, PuzzleRecord>();
  const errors: LegacyParseError[] = [];
  text.split(/\r?\n/).forEach((raw, index) => {
    const line = raw.trim();
    if (!line || line.startsWith("#") || line.startsWith("//")) return;
    const columns = raw.split("|");
    if (columns.length !== 6) {
      errors.push({ line: index + 1, message: "Expected 6 pipe-delimited columns" });
      return;
    }
    const [rawName, rawDifficulty, rawPuzzle, rawSolution, rawCreatedAt, rawSeed] = columns.map((value) => value.trim());
    try {
      const puzzle = normalizePuzzle(rawPuzzle);
      if (rawSolution && !/^[1-9]{81}$/.test(rawSolution)) throw new Error("Solution must contain 81 digits from 1 to 9");
      if (rawCreatedAt && !Number.isFinite(Date.parse(rawCreatedAt))) throw new Error("createdAt must be a valid date");
      if (rawSeed && (!/^\d+$/.test(rawSeed) || Number(rawSeed) > 0xffff_ffff)) throw new Error("seed must be a uint32 integer");
      const record: PuzzleRecord = { puzzle, source: "legacy-windows-pipe" };
      if (rawName) record.name = rawName;
      if (rawDifficulty) record.difficulty = rawDifficulty;
      if (rawSolution) record.solution = rawSolution;
      if (rawCreatedAt) record.createdAt = rawCreatedAt;
      if (rawSeed) record.seed = Number(rawSeed);
      const existing = records.get(puzzle);
      records.set(puzzle, existing ? mergePuzzleRecords(existing, record) : record);
    } catch (error) {
      errors.push({ line: index + 1, message: error instanceof Error ? error.message : String(error) });
    }
  });
  return {
    records: [...records.values()].sort((left, right) => left.puzzle.localeCompare(right.puzzle)),
    errors,
  };
}
