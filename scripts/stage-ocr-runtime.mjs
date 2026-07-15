import { cp, mkdir, readFile, readdir, rm, stat, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

async function requireFile(filePath, label) {
  try {
    if (!(await stat(filePath)).isFile()) throw new Error();
  } catch {
    throw new Error(`${label} is missing: ${filePath}`);
  }
}

export async function stageOcrRuntime({ helperPath, tessdataDir, outputDir, dllDirs = [] }) {
  await requireFile(helperPath, "OCR helper");
  const helperDir = path.dirname(helperPath);
  const capabilityPath = path.join(helperDir, "srr-ocr-capabilities.json");
  await requireFile(capabilityPath, "OCR capability manifest");
  const capabilities = JSON.parse(await readFile(capabilityPath, "utf8"));
  if (capabilities?.version !== 1 || capabilities?.ocrEnabled !== true) {
    throw new Error("OCR helper was built without OCR support; install OpenCV and Tesseract before packaging");
  }

  if (!tessdataDir) throw new Error("SRR_TESSDATA_DIR must point to a tessdata directory containing eng.traineddata");
  await requireFile(path.join(tessdataDir, "eng.traineddata"), "tessdata/eng.traineddata");

  const discovered = new Map();
  for (const directory of [helperDir, ...dllDirs]) {
    let entries;
    try { entries = await readdir(directory, { withFileTypes: true }); } catch { continue; }
    for (const entry of entries) {
      if (entry.isFile() && path.extname(entry.name).toLowerCase() === ".dll" && !discovered.has(entry.name.toLowerCase())) {
        discovered.set(entry.name.toLowerCase(), path.join(directory, entry.name));
      }
    }
  }
  if (discovered.size === 0) {
    throw new Error("No OCR runtime DLLs were discovered; set SRR_OCR_DLL_DIRS to the OpenCV/Tesseract runtime directories");
  }

  await rm(outputDir, { recursive: true, force: true });
  await mkdir(outputDir, { recursive: true });
  await cp(helperPath, path.join(outputDir, "srr-ocr-helper.exe"));
  const dlls = [];
  for (const [name, source] of [...discovered].sort(([a], [b]) => a.localeCompare(b))) {
    const filename = path.basename(source);
    await cp(source, path.join(outputDir, filename));
    dlls.push(filename);
  }
  await cp(tessdataDir, path.join(outputDir, "tessdata"), { recursive: true });
  const manifest = { version: 1, helper: "srr-ocr-helper.exe", dlls, tessdata: "tessdata/eng.traineddata" };
  await writeFile(path.join(outputDir, "runtime-manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
  return manifest;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const helperPath = path.join(repositoryRoot, "out", "build", "native-release", "srr-ocr-helper.exe");
  const outputDir = path.join(repositoryRoot, "out", "ocr-runtime");
  const dllDirs = (process.env.SRR_OCR_DLL_DIRS ?? "").split(path.delimiter).filter(Boolean);
  stageOcrRuntime({ helperPath, outputDir, tessdataDir: process.env.SRR_TESSDATA_DIR, dllDirs })
    .then((manifest) => console.log(`Staged OCR runtime with ${manifest.dlls.length} DLL(s).`))
    .catch((error) => { console.error(error instanceof Error ? error.message : error); process.exitCode = 1; });
}
