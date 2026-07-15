import assert from "node:assert/strict";
import test from "node:test";
import { IDBFactory } from "fake-indexeddb";
import {
  createBackup,
  openStorage,
  parseBackup,
  serializeBackup,
  type BackupV1,
} from "../src/index.ts";

const puzzleA = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const puzzleB = `1${"0".repeat(80)}`;

function backup(overrides: Partial<BackupV1> = {}): BackupV1 {
  return {
    schemaVersion: 1,
    puzzles: [],
    sessions: [],
    settings: [],
    exportedAt: "2026-07-15T10:00:00.000Z",
    appVersion: "0.4.0-beta.1",
    ...overrides,
  };
}

test("exports deterministic JSON with records stable-sorted by key", () => {
  const first = serializeBackup(createBackup({
    puzzles: [{ puzzle: puzzleA }, { puzzle: puzzleB }],
    sessions: [
      { id: "z", puzzle: puzzleA, values: [...puzzleA].map(Number), noteMasks: Array(81).fill(0), mode: "smart", trace: [], currentStep: 0, elapsedMs: 0, savedAt: "2026-07-15T10:00:00.000Z" },
      { id: "a", puzzle: puzzleB, values: [...puzzleB].map(Number), noteMasks: Array(81).fill(0), mode: "logic", trace: [], currentStep: 0, elapsedMs: 1, savedAt: "2026-07-15T10:00:00.000Z" },
    ],
    settings: [{ id: "z", theme: "dark" }, { id: "a", language: "en" }],
  }, { exportedAt: "2026-07-15T10:00:00.000Z", appVersion: "0.4.0-beta.1" }));
  const second = serializeBackup(createBackup({
    puzzles: [{ puzzle: puzzleB }, { puzzle: puzzleA }],
    sessions: [], settings: [],
  }, { exportedAt: "2026-07-15T10:00:00.000Z", appVersion: "0.4.0-beta.1" }));

  assert.deepEqual(JSON.parse(first).puzzles.map((entry: { puzzle: string }) => entry.puzzle), [puzzleB, puzzleA]);
  assert.deepEqual(JSON.parse(first).sessions.map((entry: { id: string }) => entry.id), ["a", "z"]);
  assert.deepEqual(JSON.parse(first).settings.map((entry: { id: string }) => entry.id), ["a", "z"]);
  assert.deepEqual(JSON.parse(second).puzzles, JSON.parse(first).puzzles);
});

test("strictly rejects unknown versions, fields, types, and ranges", () => {
  assert.throws(() => parseBackup({ ...backup(), schemaVersion: 2 }), /version/i);
  assert.throws(() => parseBackup({ ...backup(), cloudAccount: "nope" }), /unknown/i);
  assert.throws(() => parseBackup(backup({ puzzles: [{ puzzle: "123" }] })), /81/);
  assert.throws(() => parseBackup(backup({ sessions: [{
    id: "current", puzzle: puzzleA, values: Array(81).fill(10), noteMasks: Array(81).fill(0), mode: "smart", trace: [], currentStep: 0, elapsedMs: 0, savedAt: "2026-07-15T10:00:00.000Z",
  }] })), /values/i);
});

test("validates an entire backup before atomic import writes", async () => {
  const storage = await openStorage({ indexedDB: new IDBFactory(), name: "invalid-atomic" });
  await storage.upsertPuzzle({ puzzle: puzzleA, name: "Existing" });
  const invalid = {
    ...backup({ puzzles: [{ puzzle: puzzleB, name: "Must not write" }] }),
    sessions: [{ id: "bad" }],
  };

  await assert.rejects(storage.importBackup(invalid), /session/i);
  assert.deepEqual((await storage.listPuzzles()).map((entry) => entry.name), ["Existing"]);
  storage.close();
});

test("round trips and idempotently merges backup puzzle metadata", async () => {
  const storage = await openStorage({ indexedDB: new IDBFactory(), name: "round-trip" });
  await storage.upsertPuzzle({ puzzle: puzzleA, name: "Keep", updatedAt: "2026-07-15T10:00:00.000Z" });
  const input = backup({
    puzzles: [
      { puzzle: puzzleA, name: "", difficulty: "hard", updatedAt: "2026-07-14T10:00:00.000Z" },
      { puzzle: puzzleA, solution: "534678912672195348198342567859761423426853791713924856961537284287419635345286179" },
    ],
    settings: [{ id: "preferences", language: "en", theme: "dark" }],
  });

  await storage.importBackup(input);
  await storage.importBackup(input);
  const exported = parseBackup(await storage.exportBackup("0.4.0-beta.1", "2026-07-15T11:00:00.000Z"));

  assert.equal(exported.puzzles.length, 1);
  assert.deepEqual(exported.puzzles[0], {
    puzzle: puzzleA,
    name: "Keep",
    difficulty: "hard",
    solution: "534678912672195348198342567859761423426853791713924856961537284287419635345286179",
    updatedAt: "2026-07-15T10:00:00.000Z",
  });
  assert.equal(exported.settings[0]?.language, "en");
  storage.close();
});
