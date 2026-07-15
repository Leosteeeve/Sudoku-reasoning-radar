import { copyFile, mkdir } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import path from "node:path";

export async function syncRendererWasm(source, destination) {
  await mkdir(destination, { recursive: true });
  await Promise.all([
    copyFile(path.join(source, "srr-core.js"), path.join(destination, "srr-core.js")),
    copyFile(path.join(source, "srr-core.wasm"), path.join(destination, "srr-core.wasm")),
  ]);
}

const isMain = process.argv[1]
  && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url));

if (isMain) {
  const root = path.resolve(import.meta.dirname, "..");
  await syncRendererWasm(
    path.join(root, "out", "build", "wasm-release"),
    path.join(root, "apps", "renderer", "public"),
  );
}
