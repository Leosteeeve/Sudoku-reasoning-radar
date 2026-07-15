import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { access, readFile } from "node:fs/promises";
import path from "node:path";
import { promisify } from "node:util";
import test from "node:test";

const root = path.resolve(import.meta.dirname, "..");
const execFileAsync = promisify(execFile);
const desktopPackage = JSON.parse(await readFile(path.join(root, "apps", "desktop", "package.json"), "utf8"));
const rootPackage = JSON.parse(await readFile(path.join(root, "package.json"), "utf8"));
const workspace = await readFile(path.join(root, "pnpm-workspace.yaml"), "utf8");

test("desktop pins Electron 43.1 and packages both NSIS and portable Windows targets", () => {
  assert.match(desktopPackage.devDependencies.electron, /^43\.1\./);
  assert.equal(typeof desktopPackage.devDependencies["electron-builder"], "string");
  assert.deepEqual(desktopPackage.build.win.target, ["nsis", "portable"]);
  assert.equal(desktopPackage.build.publish, null);
  assert.equal(rootPackage.pnpm, undefined);
  assert.match(workspace, /allowBuilds:\s*\r?\n\s*electron: true\s*\r?\n\s*electron-winstaller: true\s*(?:\r?\n|$)/);
});

test("desktop packages the renderer build containing the same copied WASM pair", async () => {
  assert.ok(desktopPackage.build.extraResources.some((entry) => entry.from === "../renderer/dist" && entry.to === "renderer"));
  assert.match(rootPackage.scripts.build, /sync-renderer-wasm/);
  assert.match(rootPackage.scripts.build, /@srr\/renderer build/);
  await access(path.join(root, "apps", "desktop", "src", "main.ts"));
  await access(path.join(root, "apps", "desktop", "src", "preload.ts"));
});

test("sandbox preload is built as one CommonJS bundle with no local module loads", async () => {
  assert.match(desktopPackage.scripts.build, /build-preload\.mjs/);
  const main = await readFile(path.join(root, "apps", "desktop", "src", "main.ts"), "utf8");
  assert.match(main, /preload\.cjs/);
  await execFileAsync(process.execPath, [path.join(root, "apps", "desktop", "scripts", "build-preload.mjs")], {
    cwd: path.join(root, "apps", "desktop"),
  });
  const preload = await readFile(path.join(root, "apps", "desktop", "dist", "preload.cjs"), "utf8");
  assert.doesNotMatch(preload, /^\s*import\s/m);
  assert.doesNotMatch(preload, /require\(["']\.\//);
  assert.match(preload, /require\(["']electron["']\)/);
});
