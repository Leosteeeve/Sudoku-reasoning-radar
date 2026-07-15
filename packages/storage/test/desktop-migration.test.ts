import assert from "node:assert/strict";
import test from "node:test";

import { migrateLegacyDesktopPuzzles, type PuzzleRecord } from "../src/index.ts";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

function store(options: { failImport?: boolean } = {}) {
  let marked = false;
  const imported: PuzzleRecord[][] = [];
  return {
    imported,
    migrationStore: {
      async hasMigration() { return marked; },
      async importPuzzles(records: PuzzleRecord[]) {
        if (options.failImport) throw new Error("write failed");
        imported.push(records);
      },
      async markMigration() { marked = true; },
    },
  };
}

test("desktop legacy migration imports once and persists its marker only after success", async () => {
  let calls = 0;
  const target = store();
  const source = { async import() {
    calls += 1;
    return { version: 1 as const, status: "ok" as const, records: [{ puzzle, source: "legacy-windows-pipe" }], errors: [] };
  } };
  assert.deepEqual(await migrateLegacyDesktopPuzzles(source, target.migrationStore), { imported: 1, skipped: false, errors: [] });
  assert.deepEqual(target.imported, [[{ puzzle, source: "legacy-windows-pipe" }]]);
  assert.deepEqual(await migrateLegacyDesktopPuzzles(source, target.migrationStore), { imported: 0, skipped: true, errors: [] });
  assert.equal(calls, 1);
});

test("desktop legacy migration does not mark failed source or storage imports", async () => {
  const failedWrite = store({ failImport: true });
  const source = { async import() {
    return { version: 1 as const, status: "ok" as const, records: [{ puzzle }], errors: [] };
  } };
  assert.match((await migrateLegacyDesktopPuzzles(source, failedWrite.migrationStore)).errors[0] ?? "", /write failed/);
  assert.equal(await failedWrite.migrationStore.hasMigration(), false);

  const failedRead = store();
  assert.match((await migrateLegacyDesktopPuzzles({ async import() { throw new Error("bridge failed"); } }, failedRead.migrationStore)).errors[0] ?? "", /bridge failed/);
  assert.equal(await failedRead.migrationStore.hasMigration(), false);
});

test("desktop legacy not-found is a successful idempotent discovery", async () => {
  let calls = 0;
  const target = store();
  const source = { async import() { calls += 1; return { version: 1 as const, status: "not-found" as const }; } };
  assert.deepEqual(await migrateLegacyDesktopPuzzles(source, target.migrationStore), { imported: 0, skipped: false, errors: [] });
  assert.deepEqual(await migrateLegacyDesktopPuzzles(source, target.migrationStore), { imported: 0, skipped: true, errors: [] });
  assert.equal(calls, 1);
});
