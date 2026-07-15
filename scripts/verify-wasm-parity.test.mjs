import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";

import { verifyWasmParity } from "./verify-wasm-parity.mjs";

async function fixture() {
  const root = await mkdtemp(path.join(os.tmpdir(), "srr-wasm-parity-"));
  const web = path.join(root, "web");
  const desktop = path.join(root, "desktop");
  await mkdir(web);
  await mkdir(desktop);
  for (const name of ["srr-core.js", "srr-core.wasm"]) {
    await writeFile(path.join(web, name), `${name}-same`);
    await writeFile(path.join(desktop, name), `${name}-same`);
  }
  return { root, web, desktop };
}

test("Web and Electron renderer artifacts must have identical SHA-256 hashes", async () => {
  const value = await fixture();
  try {
    const hashes = await verifyWasmParity(value.web, value.desktop);
    assert.deepEqual(Object.keys(hashes), ["srr-core.js", "srr-core.wasm"]);
    assert.match(hashes["srr-core.wasm"], /^[a-f0-9]{64}$/);
    await writeFile(path.join(value.desktop, "srr-core.wasm"), "different");
    await assert.rejects(() => verifyWasmParity(value.web, value.desktop), /srr-core\.wasm.*mismatch/i);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});
