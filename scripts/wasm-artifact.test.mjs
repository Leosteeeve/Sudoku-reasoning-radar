import assert from "node:assert/strict";
import { readdir, readFile } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const artifactDirectory = process.env.SRR_WASM_DIR
  ? path.resolve(process.env.SRR_WASM_DIR)
  : path.join(repositoryRoot, "out", "build", "wasm-release");

test("WASM build emits one ES module loader and one SDL-free module", async () => {
  const entries = await readdir(artifactDirectory);
  const wasm = entries.filter((entry) => entry.endsWith(".wasm"));
  const loaders = entries.filter((entry) => entry.endsWith(".js"));
  assert.deepEqual(wasm, ["srr-core.wasm"]);
  assert.deepEqual(loaders, ["srr-core.js"]);

  const loader = await readFile(path.join(artifactDirectory, loaders[0]), "utf8");
  assert.match(loader, /export default/);
  assert.doesNotMatch(loader, /SDL/i);
  for (const symbol of ["_srr_dispatch", "_srr_free", "_malloc", "_free"]) {
    assert.match(loader, new RegExp(`Module\\["${symbol}"\\]`));
  }

  const bytes = await readFile(path.join(artifactDirectory, wasm[0]));
  const module = await WebAssembly.compile(bytes);
  const imports = WebAssembly.Module.imports(module).map(({ module: source, name }) => `${source}:${name}`);
  assert.equal(imports.some((entry) => /SDL/i.test(entry)), false, imports.join("\n"));
});
