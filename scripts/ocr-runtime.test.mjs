import assert from "node:assert/strict";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";

import { stageOcrRuntime } from "./stage-ocr-runtime.mjs";

async function fixture(ocrEnabled = true) {
  const root = await mkdtemp(path.join(os.tmpdir(), "srr-ocr-stage-"));
  const build = path.join(root, "build");
  const tessdata = path.join(root, "source-tessdata");
  await mkdir(build);
  await mkdir(tessdata);
  await writeFile(path.join(build, "srr-ocr-helper.exe"), "helper");
  await writeFile(path.join(build, "opencv_world.dll"), "dll");
  await writeFile(path.join(build, "srr-ocr-capabilities.json"), JSON.stringify({ version: 1, ocrEnabled }));
  await writeFile(path.join(tessdata, "eng.traineddata"), "traineddata");
  return { root, build, tessdata, output: path.join(root, "staged") };
}

test("OCR staging copies the helper, discovered DLLs, and validated tessdata", async () => {
  const value = await fixture();
  try {
    const manifest = await stageOcrRuntime({
      helperPath: path.join(value.build, "srr-ocr-helper.exe"),
      tessdataDir: value.tessdata,
      outputDir: value.output,
      resolveRuntimeDependencies: async () => [path.join(value.build, "opencv_world.dll")],
    });
    assert.deepEqual(manifest.dlls, ["opencv_world.dll"]);
    assert.equal(await readFile(path.join(value.output, "srr-ocr-helper.exe"), "utf8"), "helper");
    assert.equal(await readFile(path.join(value.output, "tessdata", "eng.traineddata"), "utf8"), "traineddata");
  } finally { await rm(value.root, { recursive: true, force: true }); }
});

test("OCR staging fails clearly for a stub helper or missing eng data", async () => {
  const stub = await fixture(false);
  try {
    await assert.rejects(() => stageOcrRuntime({ helperPath: path.join(stub.build, "srr-ocr-helper.exe"), tessdataDir: stub.tessdata, outputDir: stub.output, resolveRuntimeDependencies: async () => [] }), /without OCR support/i);
  } finally { await rm(stub.root, { recursive: true, force: true }); }

  const missing = await fixture();
  try {
    await rm(path.join(missing.tessdata, "eng.traineddata"));
    await assert.rejects(() => stageOcrRuntime({ helperPath: path.join(missing.build, "srr-ocr-helper.exe"), tessdataDir: missing.tessdata, outputDir: missing.output, resolveRuntimeDependencies: async () => [path.join(missing.build, "opencv_world.dll")] }), /eng\.traineddata/i);
  } finally { await rm(missing.root, { recursive: true, force: true }); }
});

test("OCR staging rejects every unresolved dependency reported by the helper scan", async () => {
  const value = await fixture();
  try {
    await assert.rejects(() => stageOcrRuntime({
      helperPath: path.join(value.build, "srr-ocr-helper.exe"),
      tessdataDir: value.tessdata,
      outputDir: value.output,
      resolveRuntimeDependencies: async () => [
        path.join(value.build, "opencv_world.dll"),
        path.join(value.build, "tesseract-missing.dll"),
      ],
    }), /tesseract-missing\.dll/i);
  } finally { await rm(value.root, { recursive: true, force: true }); }
});

test("OCR sources contain no machine-specific tessdata fallback", async () => {
  const source = await readFile(path.join(import.meta.dirname, "..", "src", "DigitRecognizer.cpp"), "utf8");
  assert.doesNotMatch(source, /D:\/MSYS2/i);
});
