import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import path from "node:path";
import test from "node:test";

const root = path.resolve(import.meta.dirname, "..");
const read = (name) => readFile(path.join(root, name), "utf8");

test("beta release configuration pins runtimes and defines offline-capable Playwright projects", async () => {
  const pkg = JSON.parse(await read("package.json"));
  assert.equal(pkg.devDependencies["@playwright/test"], "1.61.1");
  assert.equal(pkg.scripts["test:e2e"], "playwright test");

  const config = await read("playwright.config.ts");
  for (const viewport of ["360x640", "768x1024", "1280x800", "2560x1440"]) assert.match(config, new RegExp(viewport));
  assert.match(config, /vite\/bin\/vite\.js preview/);
  assert.match(config, /SRR_E2E_PORT/);
  assert.match(config, /reuseExistingServer:\s*process\.env\.SRR_E2E_REUSE_SERVER\s*===\s*["']true["']/);
});

test("CI separates always-on protocol gates from fail-closed OCR packaging", async () => {
  const workflow = await read(".github/workflows/beta-release-gates.yml");
  for (const job of ["native-cpp", "workspace", "wasm-web", "electron-security", "ocr-protocol-stub", "windows-package-ocr"]) {
    assert.match(workflow, new RegExp(`^  ${job}:`, "m"));
  }
  assert.match(workflow, /node-version:\s*["']?24\.18\.0/);
  assert.match(workflow, /version:\s*["']?11\.0\.0/);
  assert.match(workflow, /emscripten.*6\.0\.3|version:\s*["']?6\.0\.3/is);
  assert.match(workflow, /pnpm stage:ocr-runtime/);
  assert.match(workflow, /pnpm --filter @srr\/desktop package:windows/);
  assert.match(workflow, /playwright install --with-deps chromium/);
  assert.match(workflow, /playback-performance\.spec\.ts --workers=1/);
  assert.match(workflow, /mingw-w64-ucrt-x86_64-gcc/);
  assert.match(workflow, /mingw-w64-ucrt-x86_64-tesseract-data-eng/);
  assert.match(workflow, /actions\/upload-artifact@v4/);
  assert.match(workflow, /actions\/download-artifact@v4/);
  const windowsJob = workflow.slice(workflow.indexOf("  windows-package-ocr:"));
  assert.doesNotMatch(windowsJob, /^\s*- run: pnpm build\s*$/m, "Windows packaging must consume the Web job artifact instead of rebuilding without Emscripten");
  assert.match(windowsJob, /win-unpacked[\\/]resources[\\/]renderer/);
  assert.match(workflow, /SRR_VISUAL_BASELINES_READY/);
  const workspaceJob = workflow.slice(workflow.indexOf("  workspace:"), workflow.indexOf("  wasm-web:"));
  assert.doesNotMatch(workspaceJob, /scripts\/\*\.test\.mjs/, "workspace must not run build-dependent WASM tests before the WASM job");
  assert.doesNotMatch(workflow, /sign|publish|telemetry/i);
});

test("deterministic E2E gates cover parity, recovery, visuals, OCR laziness, and performance", async () => {
  const parity = await read("e2e/parity.spec.ts");
  for (const requirement of ["analyze", "timeline", "text import", "backup", "reload", "legacy", "desktop bridge", "OCR review"]) {
    assert.match(parity, new RegExp(requirement, "i"));
  }
  const visual = await read("e2e/visual.spec.ts");
  for (const state of ["English", "Chinese", "high contrast", "reduced motion", "clipping", "coordinate drift"]) {
    assert.match(visual, new RegExp(state, "i"));
  }
  const performance = await read("e2e/playback-performance.spec.ts");
  assert.match(performance, /2000/);
  assert.match(performance, /PerformanceObserver/);
  assert.match(performance, /50/);
  assert.match(performance, /dispatchCount/);

  assert.match(parity, /legacy\.import.*toHaveBeenCalled|__legacyCalls/s);
  assert.match(parity, /download.*path|readFile/s);

  const main = await read("apps/desktop/src/main.ts");
  assert.match(main, /recognize:.*recognizeWithSidecar[\s\S]*spawnHelper:\s*\(\)\s*=>\s*spawn/);
});

test("release documentation preserves legacy data and distinguishes pending environment gates", async () => {
  const docs = (await Promise.all([
    "docs/MIGRATION.md", "docs/PRIVACY.md", "docs/ROLLBACK.md", "docs/RELEASE_CHECKLIST_v0.4.0-beta.1.md",
  ].map(read))).join("\n");
  assert.match(docs, /v0\.3\.0/);
  assert.match(docs, /at least one stable release cycle/i);
  assert.match(docs, /never delete|never deletes/i);
  assert.match(docs, /verified gates/i);
  assert.match(docs, /environment-dependent pending gates/i);
  assert.match(docs, /Electron runtime launch/i);
  assert.match(docs, /installer smoke/i);
  assert.match(docs, /real OCR recognition/i);

  const buildDocs = await read("docs/BUILD.md");
  assert.doesNotMatch(buildDocs, /reserved? these commands for later|not-yet-available message/i);
});
