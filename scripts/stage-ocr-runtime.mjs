import { execFile } from "node:child_process";
import { cp, mkdir, mkdtemp, readFile, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const execFileAsync = promisify(execFile);

async function requireFile(filePath, label) {
  try {
    if (!(await stat(filePath)).isFile()) throw new Error();
  } catch {
    throw new Error(`${label} is missing: ${filePath}`);
  }
}

export async function scanRuntimeDependencies({ helperPath, dllDirs = [] }) {
  const temporary = await mkdtemp(path.join(os.tmpdir(), "srr-ocr-dependencies-"));
  const outputPath = path.join(temporary, "resolved-dependencies.txt");
  try {
    await execFileAsync("cmake", [
      `-DSRR_HELPER=${helperPath}`,
      `-DSRR_OUTPUT=${outputPath}`,
      `-DSRR_SEARCH_DIRS=${dllDirs.join(";")}`,
      "-P",
      path.join(repositoryRoot, "scripts", "scan-ocr-runtime-dependencies.cmake"),
    ]);
    const contents = await readFile(outputPath, "utf8");
    return contents.split(/\r?\n/).map((entry) => entry.trim()).filter(Boolean);
  } catch (error) {
    const detail = error && typeof error === "object" && "stderr" in error
      ? String(error.stderr).trim()
      : error instanceof Error ? error.message : String(error);
    throw new Error(`Unable to resolve every OCR runtime dependency: ${detail}`);
  } finally {
    await rm(temporary, { recursive: true, force: true });
  }
}

export async function stageOcrRuntime({
  helperPath,
  tessdataDir,
  outputDir,
  dllDirs = [],
  resolveRuntimeDependencies = scanRuntimeDependencies,
}) {
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

  const dependencies = await resolveRuntimeDependencies({ helperPath, dllDirs: [helperDir, ...dllDirs] });
  const discovered = new Map();
  for (const dependency of dependencies) {
    await requireFile(dependency, `OCR runtime dependency ${path.basename(dependency)}`);
    const name = path.basename(dependency);
    discovered.set(name.toLowerCase(), dependency);
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
