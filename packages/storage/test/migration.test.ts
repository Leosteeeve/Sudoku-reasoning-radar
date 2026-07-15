import assert from "node:assert/strict";
import test from "node:test";
import {
  migrateLegacyWebPuzzle,
  normalizePuzzle,
  openStorage,
  type MigrationStore,
} from "../src/index.ts";
import { IDBFactory } from "fake-indexeddb";

const normalized = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const legacy = normalized.replaceAll("0", ".").replace(/(.{9})/g, "$1\n");

test("normalizes a raw legacy puzzle containing dots and whitespace", () => {
  assert.equal(normalizePuzzle(legacy), normalized);
});

test("copies the Web legacy puzzle once, preserves its source, and marks only after success", async () => {
  const source = new Map([["sudoku_reasoning_radar_last", legacy]]);
  const calls: string[] = [];
  const store: MigrationStore = {
    async hasMigration() { return false; },
    async upsertPuzzle(puzzle) { calls.push(`puzzle:${puzzle.puzzle}`); },
    async markMigration(marker) { calls.push(`marker:${marker}`); },
  };

  const report = await migrateLegacyWebPuzzle(source, store);

  assert.deepEqual(report, { imported: 1, skipped: false, errors: [] });
  assert.deepEqual(calls, [`puzzle:${normalized}`, "marker:web-local-storage-v1"]);
  assert.equal(source.get("sudoku_reasoning_radar_last"), legacy);
});

test("does not write a migration marker when copying the legacy puzzle fails", async () => {
  const source = new Map([["sudoku_reasoning_radar_last", normalized]]);
  let marked = false;
  const store: MigrationStore = {
    async hasMigration() { return false; },
    async upsertPuzzle() { throw new Error("storage unavailable"); },
    async markMigration() { marked = true; },
  };

  const report = await migrateLegacyWebPuzzle(source, store);

  assert.equal(report.imported, 0);
  assert.deepEqual(report.errors, ["storage unavailable"]);
  assert.equal(marked, false);
  assert.equal(source.get("sudoku_reasoning_radar_last"), normalized);
});

test("reports invalid legacy input without marking it migrated", async () => {
  let marked = false;
  const report = await migrateLegacyWebPuzzle(new Map([["sudoku_reasoning_radar_last", "123"]]), {
    async hasMigration() { return false; },
    async upsertPuzzle() { throw new Error("must not upsert"); },
    async markMigration() { marked = true; },
  });

  assert.equal(report.imported, 0);
  assert.match(report.errors[0] ?? "", /81/);
  assert.equal(marked, false);
});

test("is idempotent with the real repository and preserves the legacy source", async () => {
  const storage = await openStorage({ indexedDB: new IDBFactory(), name: "web-migration" });
  const source = new Map([["sudoku_reasoning_radar_last", legacy]]);

  assert.equal((await migrateLegacyWebPuzzle(source, storage)).imported, 1);
  assert.deepEqual(await migrateLegacyWebPuzzle(source, storage), { imported: 0, skipped: true, errors: [] });
  assert.equal((await storage.listPuzzles()).length, 1);
  assert.equal(source.get("sudoku_reasoning_radar_last"), legacy);
  storage.close();
});
