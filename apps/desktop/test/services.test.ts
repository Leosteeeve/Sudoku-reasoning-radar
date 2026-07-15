import assert from "node:assert/strict";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";

import { createFileServices } from "../src/file-services.ts";
import { discoverLegacyPuzzles } from "../src/legacy.ts";
import { createUpdateChecker, type UpdateCache } from "../src/update.ts";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const solution = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";

test("file services scope dialogs by purpose and never accept renderer paths", async () => {
  const opens: unknown[] = [];
  const saves: unknown[] = [];
  const reads = new Map([
    ["chosen.srr.json", "{\"schemaVersion\":1}"],
  ]);
  const writes: Array<[string, string]> = [];
  const recognized: string[] = [];
  let openResult = { canceled: false, filePaths: ["grid.png"] };
  const services = createFileServices({
    showOpenDialog: async (options) => { opens.push(options); return openResult; },
    showSaveDialog: async (options) => { saves.push(options); return { canceled: false, filePath: "export.srr.json" }; },
    readTextFile: async (filePath) => reads.get(filePath) ?? "",
    writeTextFile: async (filePath, contents) => { writes.push([filePath, contents]); },
    recognize: async (filePath) => { recognized.push(filePath); return { version: 1, status: "cancelled" }; },
  });

  await services.ocrSelectAndRecognize();
  openResult = { canceled: false, filePaths: ["chosen.srr.json"] };
  assert.deepEqual(await services.backupImport(), { version: 1, status: "ok", contents: "{\"schemaVersion\":1}" });
  assert.deepEqual(await services.backupExport("{}\n"), { version: 1, status: "ok" });

  assert.deepEqual(recognized, ["grid.png"]);
  assert.deepEqual(writes, [["export.srr.json", "{}\n"]]);
  assert.deepEqual(opens, [
    { title: "Select Sudoku image", properties: ["openFile"], filters: [{ name: "Sudoku images", extensions: ["png", "jpg", "jpeg", "webp"] }] },
    { title: "Import Sudoku Reasoning Radar backup", properties: ["openFile"], filters: [{ name: "SRR backup", extensions: ["srr.json", "json"] }] },
  ]);
  assert.deepEqual(saves, [{ title: "Export Sudoku Reasoning Radar backup", defaultPath: "sudoku-reasoning-radar.srr.json", filters: [{ name: "SRR backup", extensions: ["srr.json"] }] }]);
});

test("cancelled dialogs do not read, write, or start OCR", async () => {
  let sideEffects = 0;
  const services = createFileServices({
    showOpenDialog: async () => ({ canceled: true, filePaths: [] }),
    showSaveDialog: async () => ({ canceled: true }),
    readTextFile: async () => { sideEffects += 1; return ""; },
    writeTextFile: async () => { sideEffects += 1; },
    recognize: async () => { sideEffects += 1; return { version: 1, status: "cancelled" }; },
  });
  assert.deepEqual(await services.ocrSelectAndRecognize(), { version: 1, status: "cancelled" });
  assert.deepEqual(await services.backupImport(), { version: 1, status: "cancelled" });
  assert.deepEqual(await services.backupExport("{}"), { version: 1, status: "cancelled" });
  assert.equal(sideEffects, 0);
});

test("legacy discovery is copy-only, normalized, deduplicated, and idempotent", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "srr-legacy-"));
  const legacyDir = path.join(root, "data");
  const legacyPath = path.join(legacyDir, "puzzles.txt");
  const original = [
    `Keep||${puzzle}|${solution}|2026-07-15T00:00:00.000Z|`,
    `|hard|${puzzle.replaceAll("0", ".")}|${solution}|2026-07-14T00:00:00.000Z|42`,
  ].join("\n");
  await mkdir(legacyDir);
  await writeFile(legacyPath, original);
  try {
    const first = await discoverLegacyPuzzles([root]);
    const second = await discoverLegacyPuzzles([root]);
    assert.deepEqual(second, first);
    assert.equal(first.records.length, 1);
    assert.deepEqual(first.records[0], {
      puzzle,
      name: "Keep",
      difficulty: "hard",
      solution,
      seed: 42,
      source: "legacy-windows-pipe",
      createdAt: "2026-07-14T00:00:00.000Z",
    });
    assert.equal(await readFile(legacyPath, "utf8"), original);
  } finally {
    await rm(root, { recursive: true, force: true });
  }
});

test("update checks cache for 24 hours and reject unsafe release URLs", async () => {
  let now = Date.parse("2026-07-15T00:00:00.000Z");
  let calls = 0;
  let cache: UpdateCache | undefined;
  const checker = createUpdateChecker({
    currentVersion: "0.4.0-beta.1",
    now: () => now,
    readCache: async () => cache,
    writeCache: async (value) => { cache = value; },
    fetchLatest: async () => {
      calls += 1;
      return { tag_name: "v0.4.0-beta.2", html_url: "https://github.com/example/srr/releases/tag/v0.4.0-beta.2" };
    },
  });
  const first = await checker();
  now += 23 * 60 * 60 * 1000;
  assert.deepEqual(await checker(), first);
  assert.equal(calls, 1);
  now += 60 * 60 * 1000 + 1;
  await checker();
  assert.equal(calls, 2);

  const unsafe = createUpdateChecker({
    currentVersion: "0.4.0-beta.1",
    now: () => now,
    readCache: async () => undefined,
    writeCache: async () => assert.fail("unsafe result must not be cached"),
    fetchLatest: async () => ({ tag_name: "v9.0.0", html_url: "http://example.com/download.exe" }),
  });
  assert.deepEqual(await unsafe(), { version: 1, status: "error", code: "unsafe-url" });
});

test("update availability follows stable and prerelease semantic version ordering", async () => {
  const check = (currentVersion: string, tag_name: string) => createUpdateChecker({
    currentVersion,
    now: () => Date.parse("2026-07-15T00:00:00.000Z"),
    readCache: async () => undefined,
    writeCache: async () => undefined,
    fetchLatest: async () => ({ tag_name, html_url: "https://github.com/example/srr/releases/latest" }),
  })();
  assert.equal((await check("0.4.0-beta.1", "v0.4.0")).available, true);
  assert.equal((await check("0.4.0", "v0.4.0-beta.2")).available, false);
  assert.equal((await check("0.4.0-beta.2", "v0.4.0-beta.10")).available, true);
});
