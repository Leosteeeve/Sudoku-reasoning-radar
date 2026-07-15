import { describe, expect, it, vi } from "vitest";

import { createWasmDispatcher, loadWasmCoreClient } from "../src/core/wasm-transport.ts";

const puzzle =
  "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

describe("WASM transport", () => {
  it("allocates UTF-8 input and frees both ABI-owned pointers", async () => {
    const heap = new Uint8Array(2048);
    const allocations: number[] = [];
    const requestFrees: number[] = [];
    const responseFrees: number[] = [];
    let received = "";
    const response = JSON.stringify({ ok: true });
    heap.set(new TextEncoder().encode(`${response}\0`), 512);

    const dispatcher = createWasmDispatcher({
      HEAPU8: heap,
      _malloc(size) {
        allocations.push(size);
        return 64;
      },
      _free(pointer) {
        requestFrees.push(pointer);
      },
      _srr_dispatch(pointer) {
        const end = heap.indexOf(0, pointer);
        received = new TextDecoder().decode(heap.subarray(pointer, end));
        return 512;
      },
      _srr_free(pointer) {
        responseFrees.push(pointer);
      },
    });

    const request = JSON.stringify({ operation: "solve", label: "数独" });
    await expect(dispatcher(request)).resolves.toBe(response);
    expect(received).toBe(request);
    expect(allocations).toEqual([new TextEncoder().encode(request).byteLength + 1]);
    expect(requestFrees).toEqual([64]);
    expect(responseFrees).toEqual([512]);
  });

  it("always frees the request allocation when dispatch fails", async () => {
    const requestFree = vi.fn();
    const dispatcher = createWasmDispatcher({
      HEAPU8: new Uint8Array(128),
      _malloc: () => 16,
      _free: requestFree,
      _srr_dispatch: () => {
        throw new Error("core unavailable");
      },
      _srr_free: vi.fn(),
    });

    await expect(dispatcher("{}")).rejects.toThrow("core unavailable");
    expect(requestFree).toHaveBeenCalledWith(16);
  });

  it("wraps an injected Emscripten factory in the validated CoreClient", async () => {
    const response = JSON.stringify({
      schemaVersion: 1,
      operation: "solve",
      ok: false,
      error: { code: "fixture", path: "$.puzzle", params: {} },
    });
    const heap = new Uint8Array(4096);
    heap.set(new TextEncoder().encode(`${response}\0`), 1024);
    const factory = vi.fn(async () => ({
      HEAPU8: heap,
      _malloc: () => 8,
      _free: vi.fn(),
      _srr_dispatch: () => 1024,
      _srr_free: vi.fn(),
    }));

    const client = await loadWasmCoreClient(factory);
    await expect(client.solve({ puzzle })).rejects.toMatchObject({ code: "fixture" });
    expect(factory).toHaveBeenCalledOnce();
  });
});
