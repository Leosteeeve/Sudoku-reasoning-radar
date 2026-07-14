import { spawnSync } from "node:child_process";
import process from "node:process";

function run(command, args) {
  const result = spawnSync(command, args, { cwd: process.cwd(), stdio: "inherit" });
  if (result.error) throw result.error;
  if (result.status !== 0) process.exit(result.status ?? 1);
}

if (!process.env.EMSDK) {
  console.log("EMSDK is not active; TypeScript was built and the optional WASM build was skipped.");
} else {
  run("cmake", ["--preset", "wasm-release"]);
  run("cmake", ["--build", "--preset", "wasm-release", "--target", "srr_core_wasm"]);
}
