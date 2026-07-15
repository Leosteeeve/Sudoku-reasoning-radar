import { rm } from "node:fs/promises";
import path from "node:path";

import { build } from "esbuild";

const root = path.resolve(import.meta.dirname, "..");

await build({
  entryPoints: [path.join(root, "src", "preload.ts")],
  outfile: path.join(root, "dist", "preload.cjs"),
  bundle: true,
  external: ["electron"],
  format: "cjs",
  platform: "node",
  target: "node22",
  sourcemap: true,
});

await Promise.all([
  "preload.js",
  "preload.js.map",
  "preload.d.ts",
].map((file) => rm(path.join(root, "dist", file), { force: true })));
