import assert from "node:assert/strict";
import { mkdtemp, readFile, rm, writeFile, mkdir } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";

import { syncRendererWasm } from "./sync-renderer-wasm.mjs";

const root = path.resolve(import.meta.dirname, "..");
const packageJson = JSON.parse(await readFile(path.join(root, "package.json"), "utf8"));

test("root renderer commands keep desktop-only work explicit", () => {
  assert.equal(packageJson.scripts["dev:web"], "pnpm --filter @srr/renderer dev");
  assert.match(packageJson.scripts.build, /@srr\/core-client build/);
  assert.match(packageJson.scripts.build, /@srr\/storage build/);
  assert.match(packageJson.scripts.build, /build-wasm/);
  assert.match(packageJson.scripts.build, /@srr\/renderer build/);
  assert.match(packageJson.scripts.test, /@srr\/storage test/);
  assert.match(packageJson.scripts.test, /@srr\/storage typecheck/);
  assert.match(packageJson.scripts.test, /@srr\/renderer test/);
  assert.equal(packageJson.scripts["dev:desktop"], "node scripts/not-yet-available.mjs dev:desktop");
  assert.equal(packageJson.scripts["package:windows"], "node scripts/not-yet-available.mjs package:windows");
});

test("WASM sync copies the one loader/module pair into Vite public assets", async () => {
  const temporary = await mkdtemp(path.join(os.tmpdir(), "srr-renderer-wasm-"));
  const source = path.join(temporary, "source");
  const destination = path.join(temporary, "public");
  await mkdir(source);
  await writeFile(path.join(source, "srr-core.js"), "loader");
  await writeFile(path.join(source, "srr-core.wasm"), "module");
  try {
    await syncRendererWasm(source, destination);
    assert.equal(await readFile(path.join(destination, "srr-core.js"), "utf8"), "loader");
    assert.equal(await readFile(path.join(destination, "srr-core.wasm"), "utf8"), "module");
  } finally {
    await rm(temporary, { recursive: true, force: true });
  }
});
