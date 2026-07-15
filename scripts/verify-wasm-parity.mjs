import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const artifacts = ["srr-core.js", "srr-core.wasm"];

async function sha256(filePath) {
  return createHash("sha256").update(await readFile(filePath)).digest("hex");
}

export async function verifyWasmParity(webDirectory, desktopDirectory) {
  const hashes = {};
  for (const name of artifacts) {
    const webHash = await sha256(path.join(webDirectory, name));
    const desktopHash = await sha256(path.join(desktopDirectory, name));
    if (webHash !== desktopHash) throw new Error(`${name} hash mismatch: Web=${webHash} Electron=${desktopHash}`);
    hashes[name] = webHash;
  }
  return hashes;
}

const isMain = process.argv[1] && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url));
if (isMain) {
  if (process.argv.length !== 4) {
    console.error("Usage: node scripts/verify-wasm-parity.mjs <web-renderer-dir> <electron-renderer-dir>");
    process.exitCode = 2;
  } else {
    verifyWasmParity(path.resolve(process.argv[2]), path.resolve(process.argv[3]))
      .then((hashes) => console.log(JSON.stringify(hashes, null, 2)))
      .catch((error) => { console.error(error.message); process.exitCode = 1; });
  }
}
