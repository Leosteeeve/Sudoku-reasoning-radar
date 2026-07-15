import assert from "node:assert/strict";
import test from "node:test";
import { IDBFactory } from "fake-indexeddb";
import { openStorage, openVersionedDatabase } from "../src/index.ts";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

test("opens schema version 1 with puzzles, sessions, and settings stores", async () => {
  const indexedDB = new IDBFactory();
  const storage = await openStorage({ indexedDB, name: "schema-v1" });

  assert.equal(storage.version, 1);
  assert.deepEqual(storage.storeNames, ["puzzles", "sessions", "settings"]);

  storage.close();
});

test("upserts, gets, and lists normalized puzzles by puzzle key", async () => {
  const indexedDB = new IDBFactory();
  const storage = await openStorage({ indexedDB, name: "puzzle-crud" });
  await storage.upsertPuzzle({ puzzle: puzzle.replaceAll("0", "."), name: "Starter", difficulty: "easy" });

  assert.deepEqual(await storage.getPuzzle(puzzle), {
    puzzle,
    name: "Starter",
    difficulty: "easy",
  });
  assert.deepEqual(await storage.listPuzzles(), [{ puzzle, name: "Starter", difficulty: "easy" }]);

  storage.close();
});

test("merges complementary metadata without replacing newer non-empty values", async () => {
  const storage = await openStorage({ indexedDB: new IDBFactory(), name: "puzzle-merge" });
  await storage.upsertPuzzle({ puzzle, name: "Newest name", updatedAt: "2026-07-15T10:00:00.000Z" });
  await storage.upsertPuzzle({ puzzle, name: "", difficulty: "medium", updatedAt: "2026-07-14T10:00:00.000Z" });

  assert.deepEqual(await storage.getPuzzle(puzzle), {
    puzzle,
    name: "Newest name",
    difficulty: "medium",
    updatedAt: "2026-07-15T10:00:00.000Z",
  });
  storage.close();
});

test("recovers saved current session and settings after close and reopen", async () => {
  const indexedDB = new IDBFactory();
  const first = await openStorage({ indexedDB, name: "recovery" });
  await first.saveCurrentSession({
    id: "current",
    puzzle,
    values: [...puzzle].map(Number),
    noteMasks: Array(81).fill(0),
    mode: "smart",
    trace: [],
    currentStep: 0,
    elapsedMs: 1500,
    savedAt: "2026-07-15T10:00:00.000Z",
  });
  await first.setSettings({ id: "preferences", language: "en", theme: "dark", highContrast: true, reducedMotion: false });
  first.close();

  const second = await openStorage({ indexedDB, name: "recovery" });
  assert.equal((await second.loadCurrentSession())?.elapsedMs, 1500);
  assert.deepEqual(await second.getSettings("preferences"), {
    id: "preferences", language: "en", theme: "dark", highContrast: true, reducedMotion: false,
  });
  second.close();
});

test("rolls back a failed version upgrade without deleting existing stores or data", async () => {
  const indexedDB = new IDBFactory();
  const first = await openStorage({ indexedDB, name: "rollback" });
  await first.upsertPuzzle({ puzzle, name: "Preserved" });
  first.close();

  await assert.rejects(openVersionedDatabase(indexedDB, "rollback", 2, () => {
    throw new Error("upgrade failed");
  }), /upgrade failed/);

  const reopened = await openStorage({ indexedDB, name: "rollback" });
  assert.equal((await reopened.getPuzzle(puzzle))?.name, "Preserved");
  assert.deepEqual(reopened.storeNames, ["puzzles", "sessions", "settings"]);
  reopened.close();
});

test("reports synchronous open failures and blocked upgrades", async () => {
  const throwingFactory = { open() { throw new Error("open failed"); } } as unknown as IDBFactory;
  await assert.rejects(openStorage({ indexedDB: throwingFactory }), /open failed/);

  const indexedDB = new IDBFactory();
  const held = await openStorage({ indexedDB, name: "blocked" });
  await assert.rejects(openVersionedDatabase(indexedDB, "blocked", 2, () => {}), /blocked/);
  held.close();
});

test("validates a puzzle batch before one atomic import and can clear only v1 stores for tests", async () => {
  const storage = await openStorage({ indexedDB: new IDBFactory(), name: "atomic-puzzles" });
  await storage.upsertPuzzle({ puzzle, name: "Existing" });

  await assert.rejects(storage.importPuzzles([
    { puzzle: `1${"0".repeat(80)}`, name: "Must not write" },
    { puzzle: "invalid" },
  ]), /81/);
  assert.equal((await storage.listPuzzles()).length, 1);

  await storage.importPuzzles([{ puzzle, difficulty: "hard" }, { puzzle, solution: "534678912672195348198342567859761423426853791713924856961537284287419635345286179" }]);
  assert.equal((await storage.getPuzzle(puzzle))?.difficulty, "hard");
  await storage.clearV1Stores();
  assert.deepEqual(await storage.listPuzzles(), []);
  assert.deepEqual(await storage.listSessions(), []);
  assert.deepEqual(await storage.listSettings(), []);
  storage.close();
});
