import { spawn } from "node:child_process";
import { readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { app, BrowserWindow, dialog, ipcMain } from "electron";

import { createFileServices } from "./file-services.ts";
import { registerIpcHandlers } from "./ipc.ts";
import { discoverLegacyPuzzles } from "./legacy.ts";
import { recognizeWithSidecar } from "./ocr-session.ts";
import { createUpdateChecker, type UpdateCache } from "./update.ts";
import { createWindowOptions } from "./window.ts";

const moduleDirectory = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(moduleDirectory, "../../..");
const releaseEndpoint = "https://api.github.com/repos/Leosteeeve/Sudoku-reasoning-radar/releases/latest";

function rendererIndex(): string {
  return app.isPackaged
    ? path.join(process.resourcesPath, "renderer", "index.html")
    : path.join(repositoryRoot, "apps", "renderer", "dist", "index.html");
}

function helperExecutable(): string {
  return app.isPackaged
    ? path.join(process.resourcesPath, "ocr", "srr-ocr-helper.exe")
    : path.join(repositoryRoot, "out", "build", "native-release", "srr-ocr-helper.exe");
}

async function readUpdateCache(cachePath: string): Promise<UpdateCache | undefined> {
  try { return JSON.parse(await readFile(cachePath, "utf8")) as UpdateCache; }
  catch (error) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT" || error instanceof SyntaxError) return undefined;
    throw error;
  }
}

async function createWindow(): Promise<void> {
  const window = new BrowserWindow(createWindowOptions(path.join(moduleDirectory, "preload.js")));
  window.webContents.setWindowOpenHandler(() => ({ action: "deny" }));
  window.webContents.on("will-navigate", (event, url) => {
    if (url !== window.webContents.getURL()) event.preventDefault();
  });
  window.once("ready-to-show", () => window.show());
  await window.loadFile(rendererIndex());
}

await app.whenReady();

const fileServices = createFileServices({
  showOpenDialog: (options) => dialog.showOpenDialog(options),
  showSaveDialog: (options) => dialog.showSaveDialog(options),
  readTextFile: (filePath) => readFile(filePath, { encoding: "utf8" }),
  writeTextFile: (filePath, contents) => writeFile(filePath, contents, { encoding: "utf8", flag: "w" }),
  recognize: (imagePath) => recognizeWithSidecar(imagePath, {
    spawnHelper: () => spawn(helperExecutable(), [], { stdio: ["pipe", "pipe", "pipe"], windowsHide: true }),
    timeoutMs: 30_000,
    maxOutputBytes: 256 * 1024,
  }),
});

const cachePath = path.join(app.getPath("userData"), "update-cache-v1.json");
const updateCheck = createUpdateChecker({
  currentVersion: app.getVersion(),
  now: () => Date.now(),
  readCache: () => readUpdateCache(cachePath),
  writeCache: (cache) => writeFile(cachePath, `${JSON.stringify(cache)}\n`, { encoding: "utf8", flag: "w" }),
  fetchLatest: async () => {
    const response = await fetch(releaseEndpoint, { headers: { Accept: "application/vnd.github+json" } });
    if (!response.ok) throw new Error(`GitHub Releases returned ${response.status}`);
    const value = await response.json() as Record<string, unknown>;
    return { tag_name: value.tag_name, html_url: value.html_url };
  },
});

registerIpcHandlers(ipcMain, {
  ...fileServices,
  legacyImport: async () => {
    const result = await discoverLegacyPuzzles([
      path.dirname(app.getPath("exe")),
      repositoryRoot,
    ]);
    return result.records.length === 0 && result.errors.length === 0
      ? { version: 1, status: "not-found" }
      : { version: 1, status: "ok", records: result.records, errors: result.errors };
  },
  updateCheck,
});

await createWindow();
app.on("activate", () => { if (BrowserWindow.getAllWindows().length === 0) void createWindow(); });
app.on("window-all-closed", () => { if (process.platform !== "darwin") app.quit(); });
