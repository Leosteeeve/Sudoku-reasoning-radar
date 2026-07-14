# Build and prerequisite checks

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

Activate the desired Emscripten SDK so that `EMSDK` and the Emscripten tools are
available, then run:

```sh
cmake --preset wasm-release
cmake --build --preset wasm-release
```

The preset derives its toolchain file from the active `EMSDK` environment and
contains no machine-specific SDK path.

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
