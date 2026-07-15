import { CoreClient, type CoreDispatcher } from "@srr/core-client";

export interface EmscriptenCoreModule {
  HEAPU8?: Uint8Array;
  lengthBytesUTF8?(value: string): number;
  stringToUTF8?(value: string, pointer: number, maximumBytes: number): void;
  UTF8ToString?(pointer: number): string;
  _malloc(size: number): number;
  _free(pointer: number): void;
  _srr_dispatch(requestPointer: number): number;
  _srr_free(responsePointer: number): void;
}

export type EmscriptenCoreFactory = () => Promise<EmscriptenCoreModule>;

function decodeCString(heap: Uint8Array, pointer: number): string {
  const end = heap.indexOf(0, pointer);
  if (end === -1) throw new Error("Core response was not null terminated");
  return new TextDecoder().decode(heap.subarray(pointer, end));
}

export function createWasmDispatcher(module: EmscriptenCoreModule): CoreDispatcher {
  return async (requestJson) => {
    const bytes = new TextEncoder().encode(requestJson);
    const byteLength = module.lengthBytesUTF8?.(requestJson) ?? bytes.byteLength;
    const requestPointer = module._malloc(byteLength + 1);
    if (!requestPointer) throw new Error("Unable to allocate WASM request memory");
    let responsePointer = 0;
    try {
      if (module.stringToUTF8) {
        module.stringToUTF8(requestJson, requestPointer, byteLength + 1);
      } else if (module.HEAPU8) {
        module.HEAPU8.set(bytes, requestPointer);
        module.HEAPU8[requestPointer + bytes.byteLength] = 0;
      } else {
        throw new Error("WASM module has no UTF-8 writer");
      }
      responsePointer = module._srr_dispatch(requestPointer);
      if (!responsePointer) throw new Error("Core returned an empty response pointer");
      if (module.UTF8ToString) return module.UTF8ToString(responsePointer);
      if (module.HEAPU8) return decodeCString(module.HEAPU8, responsePointer);
      throw new Error("WASM module has no UTF-8 reader");
    } finally {
      if (responsePointer) module._srr_free(responsePointer);
      module._free(requestPointer);
    }
  };
}

export async function loadWasmCoreClient(factory: EmscriptenCoreFactory): Promise<CoreClient> {
  return new CoreClient(createWasmDispatcher(await factory()));
}

type LoaderModule = { default: EmscriptenCoreFactory };

export async function loadBrowserCoreClient(): Promise<CoreClient> {
  const loaderUrl = new URL(`${import.meta.env.BASE_URL}srr-core.js`, window.location.href).href;
  const loader = await import(/* @vite-ignore */ loaderUrl) as LoaderModule;
  return loadWasmCoreClient(loader.default);
}
