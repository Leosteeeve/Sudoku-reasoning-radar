import assert from "node:assert/strict";
import { existsSync } from "node:fs";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";

import { CoreClient } from "../packages/core-client/dist/index.js";
import { createWasmDispatcher } from "../apps/renderer/src/core/wasm-transport.ts";

const loaderPath = path.resolve(import.meta.dirname, "../apps/renderer/public/srr-core.js");
const wasmPath = path.resolve(import.meta.dirname, "../apps/renderer/public/srr-core.wasm");
const artifactsExist = existsSync(loaderPath) && existsSync(wasmPath);

test("built Emscripten module dispatches through the renderer transport", { skip: !artifactsExist }, async () => {
  const loader = await import(`${pathToFileURL(loaderPath).href}?integration-test`);
  const wasmBytes = await readFile(wasmPath);
  const module = await loader.default({
    instantiateWasm(imports, receiveInstance) {
      const instance = new WebAssembly.Instance(new WebAssembly.Module(wasmBytes), imports);
      receiveInstance(instance);
      return instance.exports;
    },
  });
  const client = new CoreClient(createWasmDispatcher(module));
  const response = await client.solve({
    puzzle: "530070000600195000098000060800060003400803001700020006060000280000419005000080079",
    mode: "smart",
    includeTrace: false,
  });
  assert.equal(response.result, "unique");
  assert.match(response.solution ?? "", /^[1-9]{81}$/);
  assert.deepEqual(response.steps, []);
});
