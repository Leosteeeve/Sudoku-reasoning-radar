import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

test("offline Web shell honors the configured base and caches only app assets", async () => {
  const [config, main, manifestText, worker] = await Promise.all([
    readFile("apps/renderer/vite.config.ts", "utf8"),
    readFile("apps/renderer/src/main.tsx", "utf8"),
    readFile("apps/renderer/public/manifest.webmanifest", "utf8"),
    readFile("apps/renderer/public/sw.js", "utf8"),
  ]);
  assert.match(config, /SRR_BASE_PATH/);
  assert.match(main, /import\.meta\.env\.BASE_URL/);
  const manifest = JSON.parse(manifestText);
  assert.equal(manifest.start_url, "./");
  assert.match(worker, /srr-core\.wasm/);
  assert.match(worker, /srr-core\.js/);
  assert.doesNotMatch(worker, /indexeddb|puzzles|sessions|user-data/i);
});
