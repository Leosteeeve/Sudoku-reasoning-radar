# Build and prerequisite checks

## Supported beta toolchains and targets

- Node 24.18, pnpm 11, Electron 43.1, C++17, CMake 3.24+, and Ninja.
- Emscripten 6.0.3 at SDK release hash `9074aa513b501925adb1361e208932ad32a29a5f`.
- Windows 10 22H2 or Windows 11 x64 for NSIS/portable artifacts.
- Current two stable Chrome, Edge, and Firefox releases for the Web renderer.

Use `pnpm bootstrap`, `pnpm dev:web`, `pnpm dev:desktop`, `pnpm build`, and `pnpm test` from the repository root. After a production renderer build, `pnpm test:e2e` starts a local Vite preview server. Windows packaging is `pnpm package:windows`; OCR packaging intentionally fails when OpenCV/Tesseract DLLs or `tessdata/eng.traineddata` are missing.

The portable v1 core uses CMake, CTest, and Ninja. It does not link the legacy
SDL, OpenCV, Tesseract, or platform-specific application libraries.

## Prerequisites

- Node.js 24
- pnpm 11
- CMake 3.24 or newer
- Ninja
- a C++17 compiler
- Emscripten for the WebAssembly preset

Run the read-only diagnostic from the repository root:

```sh
pnpm run bootstrap
```

It checks every prerequisite in one pass and installs nothing. A missing
Emscripten installation makes the diagnostic exit nonzero but does not prevent
the native core tests from running.

## Native core

Configure, build, and test a debug build:

```sh
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug --output-on-failure
```

The equivalent workspace command is:

```sh
pnpm test
```

For an optimized native library, use `native-release` in place of
`native-debug`.

On Windows, run native CMake commands in a compiler-enabled developer shell.

## WebAssembly core

The release toolchain is pinned in `toolchains/emscripten.json` to Emscripten
6.0.3, SDK release hash `9074aa513b501925adb1361e208932ad32a29a5f`.
Activate that Emscripten SDK so that `EMSDK` and the Emscripten tools are
available, then run:

```sh
cmake --preset wasm-release
cmake --build --preset wasm-release --target srr_core_wasm
```

The preset derives its toolchain file from the active `EMSDK` environment and
contains no machine-specific SDK path. The target emits `srr-core.js` and
`srr-core.wasm`; it exports only `srr_dispatch`, `srr_free`, `malloc`, and
`free` and has no SDL dependency.

`pnpm build` always builds the TypeScript core client and also builds this WASM
target when `EMSDK` is active.

## Application commands

The workspace reserves these commands for later v1 tasks:

```sh
pnpm run dev:web
pnpm run dev:desktop
pnpm run build
pnpm run package:windows
```

Until their applications exist, each command prints a not-yet-available message
and exits nonzero.

## Legacy compatibility builds

The existing root and `scripts/` batch/PowerShell commands remain unchanged for
the v0.3 SDL desktop and browser editions. They are legacy compatibility paths;
new core work should use the CMake presets above.
