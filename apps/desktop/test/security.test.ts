import assert from "node:assert/strict";
import test from "node:test";

import { createPreloadApi } from "../src/preload-api.ts";
import { CHANNELS, parseRequest, parseResponse } from "../src/protocol.ts";
import { registerIpcHandlers } from "../src/ipc.ts";
import { createWindowOptions } from "../src/window.ts";

test("BrowserWindow enforces sandbox, context isolation, and no Node integration", () => {
  const options = createWindowOptions("C:\\trusted\\preload.js");
  assert.deepEqual(options.webPreferences, {
    preload: "C:\\trusted\\preload.js",
    sandbox: true,
    contextIsolation: true,
    nodeIntegration: false,
  });
});

test("preload exposes exactly five purpose-scoped methods", async () => {
  const calls: Array<[string, unknown]> = [];
  const api = createPreloadApi(async (channel, request) => {
    calls.push([channel, request]);
    if (channel === CHANNELS.ocrSelectAndRecognize) return { version: 1, status: "cancelled" };
    if (channel === CHANNELS.legacyImport) return { version: 1, status: "not-found" };
    if (channel === CHANNELS.backupImport) return { version: 1, status: "cancelled" };
    if (channel === CHANNELS.backupExport) return { version: 1, status: "ok" };
    return { version: 1, status: "ok", checkedAt: "2026-07-15T00:00:00.000Z", available: false };
  });

  assert.deepEqual(Object.keys(api).sort(), ["backup", "legacy", "ocr", "update"]);
  assert.deepEqual(Object.keys(api.ocr), ["selectAndRecognize"]);
  assert.deepEqual(Object.keys(api.legacy), ["import"]);
  assert.deepEqual(Object.keys(api.backup).sort(), ["export", "import"]);
  assert.deepEqual(Object.keys(api.update), ["check"]);
  assert.equal("fs" in api, false);
  assert.equal("process" in api, false);
  assert.equal("shell" in api, false);
  assert.equal("ipc" in api, false);

  await api.ocr.selectAndRecognize();
  await api.legacy.import();
  await api.backup.import();
  await api.backup.export("{}\n");
  await api.update.check();
  assert.deepEqual(calls, [
    [CHANNELS.ocrSelectAndRecognize, { version: 1 }],
    [CHANNELS.legacyImport, { version: 1 }],
    [CHANNELS.backupImport, { version: 1 }],
    [CHANNELS.backupExport, { version: 1, contents: "{}\n" }],
    [CHANNELS.updateCheck, { version: 1 }],
  ]);
});

test("strict request validation rejects unknown versions, fields, and arbitrary paths", () => {
  assert.throws(() => parseRequest(CHANNELS.ocrSelectAndRecognize, { version: 2 }), /version/i);
  assert.throws(() => parseRequest(CHANNELS.ocrSelectAndRecognize, { version: 1, path: "C:\\secrets.txt" }), /unknown/i);
  assert.throws(() => parseRequest(CHANNELS.backupImport, { version: 1, filePath: "C:\\secrets.txt" }), /unknown/i);
  assert.throws(() => parseRequest(CHANNELS.backupExport, { version: 1, contents: "{}", path: "C:\\overwrite.txt" }), /unknown/i);
  assert.throws(() => parseRequest("srr:process:spawn", { version: 1 }), /channel/i);
});

test("strict response validation rejects malformed nested IPC data", () => {
  const validCell = { digit: 0, confidence: 90, lowConfidence: false };
  const cells = Array.from({ length: 81 }, () => ({ ...validCell }));
  assert.throws(() => parseResponse(CHANNELS.ocrSelectAndRecognize, {
    version: 1, status: "ok", puzzle: "0".repeat(81), cells: [{ ...validCell, path: "C:\\secret" }, ...cells.slice(1)],
  }), /unknown/i);
  assert.throws(() => parseResponse(CHANNELS.ocrSelectAndRecognize, {
    version: 1, status: "ok", puzzle: "1" + "0".repeat(80), cells,
  }), /puzzle/i);
  assert.throws(() => parseResponse(CHANNELS.legacyImport, {
    version: 1, status: "ok", records: [{ puzzle: "0".repeat(81), filePath: "C:\\secret" }], errors: [],
  }), /unknown/i);
  assert.throws(() => parseResponse(CHANNELS.legacyImport, {
    version: 1, status: "ok", records: [], errors: [{ line: 0, message: "bad" }],
  }), /line/i);
  assert.throws(() => parseResponse(CHANNELS.updateCheck, {
    version: 1, status: "ok", checkedAt: "yesterday", available: false,
  }), /checkedAt/i);
});

test("IPC registration installs only the fixed allowlist", () => {
  const registered = new Map<string, (event: unknown, request: unknown) => Promise<unknown>>();
  const ipcMain = { handle(channel: string, handler: (event: unknown, request: unknown) => Promise<unknown>) { registered.set(channel, handler); } };
  registerIpcHandlers(ipcMain, {
    ocrSelectAndRecognize: async () => ({ version: 1, status: "cancelled" }),
    legacyImport: async () => ({ version: 1, status: "not-found" }),
    backupImport: async () => ({ version: 1, status: "cancelled" }),
    backupExport: async () => ({ version: 1, status: "ok" }),
    updateCheck: async () => ({ version: 1, status: "ok", checkedAt: "2026-07-15T00:00:00.000Z", available: false }),
  });
  assert.deepEqual([...registered.keys()].sort(), Object.values(CHANNELS).sort());
  assert.equal(registered.has("srr:process:spawn"), false);
});
